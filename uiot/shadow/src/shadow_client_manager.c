/*
* Copyright (C) 2012-2019 UCloud. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License").
* You may not use this file except in compliance with the License.
* A copy of the License is located at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* or in the "license" file accompanying this file. This file is distributed
* on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
* express or implied. See the License for the specific language governing
* permissions and limitations under the License.
*/


#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "shadow_client.h"
#include "shadow_client_common.h"
#include "uiot_internal.h"
#include "uiot_import.h"

#include "shadow_client_json.h"
#include "utils_list.h"

#define min(a,b) (a) < (b) ? (a) : (b)

/**
 * @brief 代表一个设备修改设备影子文档的请求
 */
typedef struct {
    Method                 method;                 // 文档操作方式
    List                   *property_delta_list;   // 该请求需要修改的属性，期望值，时间戳
    void                   *user_context;          // 用户数据
    Timer                  timer;                  // 请求超时定时器
    OnRequestCallback      callback;               // 文档操作请求返回处理函数
} Request;

static char cloud_rcv_buf[CLOUD_IOT_JSON_RX_BUF_LEN];

static Method cloud_send_method;

typedef void (*TraverseHandle)(UIoT_Shadow *pShadow, ListNode **node, List *list, const char *pType);

static int shadow_json_init(char *pJsonDoc, size_t sizeOfBuffer, RequestParams *pParams);

static int shadow_json_set_content(char *pJsonDoc, size_t sizeOfBuffer, RequestParams *pParams);

static int shadow_json_finalize(UIoT_Shadow *pShadow, char *pJsonDoc, size_t sizeOfBuffer, RequestParams *pParams);

static int uiot_shadow_publish_operation_to_cloud(UIoT_Shadow *pShadow, Method method, char *pJsonDoc);

static int shadow_add_request_to_list(UIoT_Shadow *pShadow, RequestParams *pParams);

static int uiot_shadow_unsubscribe_topic(UIoT_Shadow *pShadow, char *topicFilter);

static void uiot_shadow_request_destory(void *request);

int uiot_shadow_init(UIoT_Shadow *pShadow) 
{
    FUNC_ENTRY;

    POINTER_VALID_CHECK(pShadow, ERR_PARAM_INVALID);

    pShadow->inner_data.version = 0;

    pShadow->request_mutex = HAL_MutexCreate();
    if (pShadow->request_mutex == NULL)
        FUNC_EXIT_RC(FAILURE_RET);

    pShadow->property_mutex = HAL_MutexCreate();
    if (pShadow->property_mutex == NULL)
        FUNC_EXIT_RC(FAILURE_RET);

    pShadow->inner_data.property_list = list_new();
    if (pShadow->inner_data.property_list)
    {
        pShadow->inner_data.property_list->free = HAL_Free;
        pShadow->inner_data.property_list->match = shadow_common_check_property_match;
    }
    else 
    {
        LOG_ERROR("no memory to allocate property_handle_list\n");
        FUNC_EXIT_RC(FAILURE_RET);
    }

    pShadow->inner_data.request_list = list_new();
    if (pShadow->inner_data.request_list)
    {
        pShadow->inner_data.request_list->free = HAL_Free;
    } 
    else 
    {
        LOG_ERROR("no memory to allocate request_list\n");
        FUNC_EXIT_RC(FAILURE_RET);
    }

    FUNC_EXIT_RC(SUCCESS_RET);
}


void uiot_shadow_reset(void *pClient) 
{
    FUNC_ENTRY;

    POINTER_VALID_CHECK_RTN(pClient);

    UIoT_Shadow *shadow_client = (UIoT_Shadow *)pClient;
    if (shadow_client->inner_data.property_list) 
    {
        list_destroy(shadow_client->inner_data.property_list);
    }

    uiot_shadow_unsubscribe_topic(shadow_client,SHADOW_SUBSCRIBE_REQUEST_TEMPLATE);
    uiot_shadow_unsubscribe_topic(shadow_client,SHADOW_SUBSCRIBE_SYNC_TEMPLATE);
    
    if (shadow_client->inner_data.request_list)
    {
        list_destroy(shadow_client->inner_data.request_list);
    }
}

int uiot_shadow_update_property(UIoT_Shadow *pShadow, RequestParams *pParams)
{
    FUNC_ENTRY;
    
    POINTER_VALID_CHECK(pShadow, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pShadow->property_mutex, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pParams, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pParams->property_delta_list, ERR_PARAM_INVALID);

    int ret = 0;
    
    if ((pParams->property_delta_list->len) && (DELETE_ALL != pParams->method))
    {
        ListIterator *iter;
        ListNode *node = NULL;
        DeviceProperty *pProperty = NULL;
    
        if (NULL == (iter = list_iterator_new(pParams->property_delta_list, LIST_TAIL))) 
        {   
            ret = ERR_PARAM_INVALID;
            LOG_ERROR("property_delta_list is null\n");
            FUNC_EXIT_RC(ret);
        }
    
        for (;;) 
        {
            node = list_iterator_next(iter);
            if (NULL == node) {
                break;
            }
    
            pProperty = (DeviceProperty *)(node->val);
            if (NULL == pProperty) 
            {
                LOG_ERROR("node's value is invalid!\n");
                continue;
            }

            shadow_common_update_property(pShadow, pProperty, pParams->method);                                         
        }
    
        list_iterator_destroy(iter);
    }            

    /* 将所有本地属性置为空 */
    if(DELETE_ALL == pParams->method)
    {
        HAL_MutexLock(pShadow->property_mutex);
        if (pShadow->inner_data.property_list->len) 
        {
            ListIterator *shadow_iter;
            ListNode *shadow_node = NULL;
            PropertyHandler *property_handle = NULL;
            DeviceProperty *shadow_property = NULL;
        
            if (NULL == (shadow_iter = list_iterator_new(pShadow->inner_data.property_list, LIST_TAIL))) 
            {   
                ret = ERR_PARAM_INVALID;
                LOG_ERROR("property_list is null\n");
                FUNC_EXIT_RC(ret);
            }
        
            for (;;) 
            {
                shadow_node = list_iterator_next(shadow_iter);
                if (NULL == shadow_node) {
                    break;
                }
        
                property_handle = (PropertyHandler *)(shadow_node->val);
                shadow_property = (DeviceProperty *)property_handle->property;
                if (NULL == shadow_property) 
                {
                    LOG_ERROR("node's value is invalid!\n");
                    continue;
                }
                shadow_property->data = NULL;
            }
            list_iterator_destroy(shadow_iter);
        }     
        HAL_MutexUnlock(pShadow->property_mutex);
    }

    FUNC_EXIT_RC(ret);

}

int uiot_shadow_make_request(UIoT_Shadow *pShadow,char *pJsonDoc, size_t sizeOfBuffer, RequestParams *pParams)
{
    FUNC_ENTRY;

    POINTER_VALID_CHECK(pShadow, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pParams, ERR_PARAM_INVALID);

    //把请求加入设备的请求队列中
    int ret = SUCCESS_RET;
    ret = shadow_add_request_to_list(pShadow, pParams);

    ret = shadow_json_init(pJsonDoc, sizeOfBuffer, pParams);
    if (ret != SUCCESS_RET)
    {
        goto end;
    }

    ret = shadow_json_set_content(pJsonDoc, sizeOfBuffer, pParams);
    if (ret != SUCCESS_RET)
    {
        goto end;
    }

    ret = shadow_json_finalize(pShadow, pJsonDoc, sizeOfBuffer, pParams);
    if (ret != SUCCESS_RET)
    {
        goto end;
    }

    LOG_DEBUG("jsonDoc: %s\n", pJsonDoc);

    // 相应的 operation topic 订阅成功或已经订阅
    ret = uiot_shadow_publish_operation_to_cloud(pShadow, pParams->method, pJsonDoc);
    if (ret != SUCCESS_RET)
    {
        goto end;
    }

    // 向云平台发送成功更新请求后,根据请求同步更新本地属性
    ret = uiot_shadow_update_property(pShadow, pParams);
    if (ret != SUCCESS_RET)
    {
        goto end;
    }

end:
    uiot_shadow_request_destory(pParams);
    FUNC_EXIT_RC(ret);
}

int uiot_shadow_subscribe_topic(UIoT_Shadow *pShadow, char *topicFilter, OnMessageHandler on_message_handler)
{
    FUNC_ENTRY;
    
    int ret;
    UIoT_Client *mqtt_client = (UIoT_Client *)pShadow->mqtt;

    char *topic_name = (char *)HAL_Malloc(MAX_SIZE_OF_CLOUD_TOPIC * sizeof(char));
    if (topic_name == NULL) 
    {
        LOG_ERROR("topic_name malloc fail\n");
        FUNC_EXIT_RC(FAILURE_RET);
    }
    memset(topic_name, 0x0, MAX_SIZE_OF_CLOUD_TOPIC);
    HAL_Snprintf(topic_name, MAX_SIZE_OF_CLOUD_TOPIC, topicFilter, pShadow->product_sn, pShadow->device_sn);    
    
    SubscribeParams subscribe_params = DEFAULT_SUB_PARAMS;
    subscribe_params.on_message_handler = on_message_handler;
    subscribe_params.qos = QOS1;

    ret = IOT_MQTT_Subscribe(mqtt_client, topic_name, &subscribe_params);
    if (ret < 0) 
    {
        LOG_ERROR("subscribe topic: %s failed: %d.\n", topicFilter, ret);
    }

    HAL_Free(topic_name);
    FUNC_EXIT_RC(ret);
}

static int uiot_shadow_publish_operation_to_cloud(UIoT_Shadow *pShadow, Method method, char *pJsonDoc)
{
    FUNC_ENTRY;

    int ret = SUCCESS_RET;

    char topic[MAX_SIZE_OF_CLOUD_TOPIC] = {0};
    int size;

    if(GET == method)
    {
        size = HAL_Snprintf(topic, MAX_SIZE_OF_CLOUD_TOPIC, SHADOW_PUBLISH_SYNC_TEMPLATE, pShadow->product_sn, pShadow->device_sn);   
    }
    else
    {
        size = HAL_Snprintf(topic, MAX_SIZE_OF_CLOUD_TOPIC, SHADOW_PUBLISH_REQUEST_TEMPLATE, pShadow->product_sn, pShadow->device_sn);    
    }
    
    if (size < 0 || size > MAX_SIZE_OF_CLOUD_TOPIC - 1)
    {
        LOG_ERROR("buf size < topic length!\n");
        FUNC_EXIT_RC(FAILURE_RET);
    }

    cloud_send_method = method;
    PublishParams pubParams = DEFAULT_PUB_PARAMS;
    pubParams.qos = QOS1;
    pubParams.payload_len = strlen(pJsonDoc);
    pubParams.payload = (char *) pJsonDoc;

    ret = IOT_MQTT_Publish(pShadow->mqtt, topic, &pubParams);
    if(ret >= 0)
    {        
        FUNC_EXIT_RC(SUCCESS_RET);
    }
    else
    {
        FUNC_EXIT_RC(FAILURE_RET);
    }
}

static int uiot_shadow_unsubscribe_topic(UIoT_Shadow *pShadow, char *topicFilter)
{
    FUNC_ENTRY;
    POINTER_VALID_CHECK(topicFilter, ERR_PARAM_INVALID);
    
    int ret;
    UIoT_Client *mqtt_client = (UIoT_Client *)pShadow->mqtt;

    char *topic_name = (char *)HAL_Malloc(MAX_SIZE_OF_CLOUD_TOPIC * sizeof(char));
    if (topic_name == NULL) 
    {
        LOG_ERROR("topic_name malloc fail\n");
        FUNC_EXIT_RC(FAILURE_RET);
    }
    memset(topic_name, 0x0, MAX_SIZE_OF_CLOUD_TOPIC);
    HAL_Snprintf(topic_name, MAX_SIZE_OF_CLOUD_TOPIC, topicFilter, pShadow->product_sn, pShadow->device_sn);    
    
    ret = IOT_MQTT_Unsubscribe(mqtt_client, topic_name);
    if (ret < 0) 
    {
        LOG_ERROR("unsubscribe topic: %s failed: %d.\n", topicFilter, ret);
    }
    
    HAL_Free(topic_name);
    FUNC_EXIT_RC(ret);
}

void* uiot_shadow_request_init(Method method, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context)
{
    FUNC_ENTRY;

    RequestParams *pParams = NULL;
    
    if((pParams = (RequestParams *) HAL_Malloc (sizeof(RequestParams))) == NULL)
    {
        LOG_ERROR("malloc RequestParams error\n");
        return NULL;
    }
    pParams->method = method;    
    pParams->property_delta_list = list_new();
    if (pParams->property_delta_list)
    {
        pParams->property_delta_list->free = HAL_Free;
    }
    else 
    {
        LOG_ERROR("no memory to allocate property_delta_list\n");
        HAL_Free(pParams);
        return NULL;
    }
    pParams->request_callback = request_callback;
    pParams->timeout_sec = timeout_sec;
    pParams->user_context = user_context;
    
    return pParams;
}

void uiot_shadow_request_destory(void *request) 
{
    FUNC_ENTRY;

    POINTER_VALID_CHECK_RTN(request);

    RequestParams *shadow_request = (RequestParams *)request;

    unsigned int len = shadow_request->property_delta_list->len;
    ListNode *next;
    ListNode *curr = shadow_request->property_delta_list->head;

    while (len--) {
        next = curr->next;
        HAL_Free(curr);
        curr = next;
    }

    HAL_Free(shadow_request->property_delta_list);
    
    HAL_Free(request);
}

/**
 * @brief 处理注册属性的回调函数
 * 当订阅的$system/{ProductId}/{DeviceName}/shadow/downstream
 * 和$system/{ProductId}/{DeviceName}/shadow/getreply返回消息时调用
 */
static void _handle_delta(UIoT_Shadow *pShadow, char* delta_str)
{
    FUNC_ENTRY;

    char *property_value = NULL;       
    RequestParams *pParams_property = NULL;    
    char JsonDoc[CLOUD_IOT_JSON_RX_BUF_LEN];
    size_t sizeOfBuffer = sizeof(JsonDoc) / sizeof(JsonDoc[0]);

    if((UPDATE == cloud_send_method) 
        || (GET == cloud_send_method) 
        || (UPDATE_AND_RESET_VER == cloud_send_method) 
        || (REPLY_CONTROL_UPDATE == cloud_send_method))
    {
        pParams_property = (RequestParams *)uiot_shadow_request_init(REPLY_CONTROL_UPDATE, NULL, MAX_WAIT_TIME_SEC, NULL);
    }
    else if((DELETE == cloud_send_method) 
        || (DELETE_ALL == cloud_send_method) 
        || (REPLY_CONTROL_DELETE == cloud_send_method))
    {
        pParams_property = (RequestParams *)uiot_shadow_request_init(REPLY_CONTROL_DELETE, NULL, MAX_WAIT_TIME_SEC, NULL);
    }
    
    HAL_MutexLock(pShadow->property_mutex);
    if (pShadow->inner_data.property_list->len) 
    {
        ListIterator *iter;
        ListNode *node = NULL;
        PropertyHandler *property_handle = NULL;

        if (NULL == (iter = list_iterator_new(pShadow->inner_data.property_list, LIST_TAIL))) 
        {
            FUNC_EXIT;
        }

        for (;;) 
        {
            node = list_iterator_next(iter);
            if (NULL == node) 
            {
                break;
            }
            
            property_handle = (PropertyHandler *)(node->val);
            if (NULL == property_handle) {
                LOG_ERROR("node's value is invalid!\n");
                continue;
            }
            
            if (property_handle->property != NULL) 
            {

                if (NULL != (property_value = find_value_if_key_match(delta_str, property_handle->property)))
                {   
                    if (property_handle->callback != NULL)
                    {
                        property_handle->callback(pShadow, pParams_property, property_value, strlen(property_value), property_handle->property);
                    }
                    node = NULL;
                }
            }
            
        }
        
        list_iterator_destroy(iter);
    }
    HAL_MutexUnlock(pShadow->property_mutex);
    
    /* 根据回复的设备影子文档中Desired字段的属性值修改完后,把更新完的结果回复服务器 */
    uiot_shadow_make_request(pShadow, JsonDoc, sizeOfBuffer, pParams_property);

    HAL_Free(property_value);
    FUNC_EXIT;
}

/**
 * @brief 遍历列表, 对列表每个节点都执行一次传入的函数traverseHandle
 */
static void _traverse_list(UIoT_Shadow *pShadow, List *list, const char *pType, TraverseHandle traverseHandle)
{
    FUNC_ENTRY;

    HAL_MutexLock(pShadow->request_mutex);

    LOG_DEBUG("request list len:%d\n",list->len);

    if (list->len) 
    {
        ListIterator *iter;
        ListNode *node = NULL;

        if (NULL == (iter = list_iterator_new(list, LIST_TAIL))) 
        {
            HAL_MutexUnlock(pShadow->request_mutex);
            FUNC_EXIT;
        }

        for (;;) 
        {
            node = list_iterator_next(iter);
            if (NULL == node) 
            {
                break;
            }

            if (NULL == node->val) {
                LOG_DEBUG("node's value is invalid!\n");
                continue;
            }

            traverseHandle(pShadow, &node, list, pType);
        }

        list_iterator_destroy(iter);
    }
    HAL_MutexUnlock(pShadow->request_mutex);

    FUNC_EXIT;
}

/**
 * @brief 执行设备影子操作的回调函数
 */
static void _handle_request_callback(UIoT_Shadow *pShadow, ListNode **node, List *list, const char *pType)
{
    FUNC_ENTRY;

    Request *request = (Request *)(*node)->val;
    if (NULL == request)
        FUNC_EXIT;

    RequestAck status = ACK_NONE;

    uint32_t result_code = 0;
    if(!strcmp(pType, METHOD_GET_REPLY))
    {
        status = ACK_ACCEPTED;
    }
    else if((!strcmp(pType, METHOD_REPLY)) || (!strcmp(pType, METHOD_CONTROL)))
    {
        bool parse_success = parse_shadow_payload_retcode_type(cloud_rcv_buf, &result_code);
        if (parse_success) 
        {
            if (result_code == 0) {
                status = ACK_ACCEPTED;
            } else {
                status = ACK_REJECTED;
            }
        }
        else 
        {
            LOG_ERROR("parse shadow operation result code failed.\n");
            status = ACK_REJECTED;
        }
    }
    
    if (request->callback != NULL) 
    {
        request->callback(pShadow, request->method, status, cloud_rcv_buf, request->user_context);
    }
    
    list_remove(list, *node);
    *node = NULL;

    FUNC_EXIT;
}

/**
 * @brief 执行过期的设备影子操作的回调函数
 */
static void _handle_expired_request_callback(UIoT_Shadow *pShadow, ListNode **node, List *list, const char *pType)
{
    FUNC_ENTRY;

    Request *request = (Request *)(*node)->val;
    if (NULL == request)
        FUNC_EXIT;

    if (has_expired(&request->timer))
    {
        if (request->callback != NULL) 
        {
            request->callback(pShadow, request->method, ACK_TIMEOUT, cloud_rcv_buf, request->user_context);
        }

        list_remove(list, *node);
        
        *node = NULL;
    }

    FUNC_EXIT;
}

void _handle_expired_request(UIoT_Shadow *pShadow) 
{
    FUNC_ENTRY;

    _traverse_list(pShadow, pShadow->inner_data.request_list, NULL, _handle_expired_request_callback);

    FUNC_EXIT;
}

/**
 * @brief 文档操作请求结果的回调函数
 * 客户端先订阅 $system/${ProductSN}/${DeviceSN}/shadow/downstream, 收到该topic的消息则会调用该回调函数
 * 在这个回调函数中, 解析出各个设备影子文档操作的结果
 */
void topic_request_result_handler(void *pClient, MQTTMessage *message, void *pUserdata)
{
    FUNC_ENTRY;

    POINTER_VALID_CHECK_RTN(pClient);
    POINTER_VALID_CHECK_RTN(message);

    UIoT_Client *mqtt_client = (UIoT_Client *)pClient;
    UIoT_Shadow *shadow_client = (UIoT_Shadow*)mqtt_client->event_handler.context;

    const char *topic = message->topic;
    size_t topic_len = message->topic_len;
    if (NULL == topic || topic_len <= 0) 
    {
        FUNC_EXIT;
    }

    char *method_str = NULL;
    uint32_t ret_code = 0;

    if (message->payload_len > CLOUD_IOT_JSON_RX_BUF_LEN) 
    {
        LOG_ERROR("The length of the received message exceeds the specified length!\n");
        goto end;
    }

    int cloud_rcv_len = min(CLOUD_IOT_JSON_RX_BUF_LEN - 1, message->payload_len);
    memcpy(cloud_rcv_buf, message->payload, cloud_rcv_len + 1);
    cloud_rcv_buf[cloud_rcv_len] = '\0';    // jsmn_parse relies on a string

    LOG_DEBUG("downstream get message:%s\n",cloud_rcv_buf);

    //解析shadow result topic消息类型
    if (!parse_shadow_method_type(cloud_rcv_buf, &method_str))
    {
        LOG_ERROR("Fail to parse method type!\n");
        goto end;
    }

    uint32_t version_num = 0;
    if (parse_version_num(cloud_rcv_buf, &version_num)) 
    {
        shadow_client->inner_data.version = version_num;
        LOG_DEBUG("update version:%d\n",version_num);
    }   
    else
    {
        LOG_ERROR("Fail to parse version num!\n");
        goto end;
    }

    //属性更新或者删除成功，更新本地维护的版本号
    if (!strcmp(method_str, METHOD_REPLY)) 
    {
        if (!parse_shadow_payload_retcode_type(cloud_rcv_buf, &ret_code))
        {
            LOG_ERROR("Fail to parse RetCode!\n");
            goto end;
        }    

        if(SUCCESS_RET != ret_code)
        {
            LOG_DEBUG("update or delete fail! reply:%s\n",cloud_rcv_buf);
            goto end;
        }
        
    }
    else if(!strcmp(method_str, METHOD_CONTROL))     //版本号与影子文档不符,重新同步属性
    {        
        char* desired_str = NULL;
        if (parse_shadow_payload_state_desired_state(cloud_rcv_buf, &desired_str)) 
        {
            LOG_DEBUG("desired:%s\n", desired_str);
            /* desired中的字段不为空 */
            if(strlen(desired_str) > 2)
            {
                _handle_delta(shadow_client, desired_str);
            }
            HAL_Free(desired_str);
        }
        
    }
    else
    {
        goto end;
    }

    if (shadow_client != NULL)
        _traverse_list(shadow_client, shadow_client->inner_data.request_list, method_str, _handle_request_callback);
end:
    HAL_Free(method_str);

    FUNC_EXIT;
}

/**
 * @brief 文档操作请求结果的回调函数
 * 客户端先订阅 $system/${ProductSN}/${DeviceSN}/shadow/get_reply,
    $system/${ProductSN}/${DeviceSN}/shadow/document,收到该topic的消息则会调用该回调函数
 * 在这个回调函数中, 解析出各个设备影子文档操作的结果
 */
void topic_sync_handler(void *pClient, MQTTMessage *message, void *pUserdata)
{
    FUNC_ENTRY;

    POINTER_VALID_CHECK_RTN(pClient);
    POINTER_VALID_CHECK_RTN(message);

    UIoT_Client *mqtt_client = (UIoT_Client *)pClient;
    UIoT_Shadow *shadow_client = (UIoT_Shadow*)mqtt_client->event_handler.context;

    const char *topic = message->topic;
    size_t topic_len = message->topic_len;
    if (NULL == topic || topic_len <= 0) 
    {
        FUNC_EXIT;
    }
    
    if (message->payload_len > CLOUD_IOT_JSON_RX_BUF_LEN) 
    {
        LOG_ERROR("The length of the received message exceeds the specified length!\n");
        goto end;
    }

    int cloud_rcv_len = min(CLOUD_IOT_JSON_RX_BUF_LEN - 1, message->payload_len);
    memcpy(cloud_rcv_buf, message->payload, cloud_rcv_len + 1);
    cloud_rcv_buf[cloud_rcv_len] = '\0';    // jsmn_parse relies on a string

    LOG_DEBUG("get_reply:%s\n",cloud_rcv_buf);

    //同步返回消息中的version
    uint32_t version_num = 0;
    if (parse_version_num(cloud_rcv_buf, &version_num)) {
        shadow_client->inner_data.version = version_num;
    }
    else
    {
        LOG_ERROR("Fail to parse version num!\n");
        goto end;
    }

    LOG_DEBUG("version num:%d\n",shadow_client->inner_data.version);

    char* desired_str = NULL;
    if (parse_shadow_state_desired_type(cloud_rcv_buf, &desired_str)) 
    {
        LOG_DEBUG("Desired part:%s\n", desired_str);
        /* desired中的字段不为空       */
        if(strlen(desired_str) > 2)
        {
            _handle_delta(shadow_client, desired_str);
        }
        HAL_Free(desired_str);
    }

    if (shadow_client != NULL)
        _traverse_list(shadow_client, shadow_client->inner_data.request_list, METHOD_GET_REPLY, _handle_request_callback);
end:

    FUNC_EXIT;
}

/**
 * @brief 根据RequestParams来给json填入type字段
 */
static int shadow_json_init(char *pJsonDoc, size_t sizeOfBuffer, RequestParams *pParams)
{
    FUNC_ENTRY;

    POINTER_VALID_CHECK(pJsonDoc, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pParams, ERR_PARAM_INVALID);
    
    int ret = SUCCESS_RET;
    char *type_str = NULL;
    
    switch (pParams->method) 
    {
      case GET:
        type_str = METHOD_GET;
        HAL_Snprintf(pJsonDoc, sizeOfBuffer, "{\"Method\":\"%s\"", type_str);
        break;
      case UPDATE:
      case UPDATE_AND_RESET_VER:
        type_str = METHOD_UPDATE;
        HAL_Snprintf(pJsonDoc, sizeOfBuffer, "{\"Method\":\"%s\",\"State\":{\"Reported\":{", type_str);
        break;
      case REPLY_CONTROL_UPDATE:
        type_str = METHOD_UPDATE;
        if (pParams->property_delta_list->len) 
        {   
            /* 没有完全按照Desired字段更新属性 */
            HAL_Snprintf(pJsonDoc, sizeOfBuffer, "{\"Method\":\"%s\",\"State\":{\"Reported\":{", type_str);
        }
        else
        {   
            /* 完全按照Desired字段更新属性 */
            HAL_Snprintf(pJsonDoc, sizeOfBuffer, "{\"Method\":\"%s\",\"State\":{", type_str);
        }
        break;
      case DELETE:
        type_str = METHOD_DELETE;
        if (pParams->property_delta_list->len) 
        {   
            /* 没有完全按照Desired字段更新属性 */
            HAL_Snprintf(pJsonDoc, sizeOfBuffer, "{\"Method\":\"%s\",\"State\":{\"Reported\":{", type_str);
        }
        else
        {   
            /* 完全按照Desired字段更新属性 */
            HAL_Snprintf(pJsonDoc, sizeOfBuffer, "{\"Method\":\"%s\",\"State\":{", type_str);
        }
        break;      
      case REPLY_CONTROL_DELETE:
        type_str = METHOD_DELETE;
        if (pParams->property_delta_list->len) 
        {
            HAL_Snprintf(pJsonDoc, sizeOfBuffer, "{\"Method\":\"%s\",\"State\":{\"Reported\":{", type_str);
        }
        else
        {
            HAL_Snprintf(pJsonDoc, sizeOfBuffer, "{\"Method\":\"%s\",\"State\":{", type_str);
        }
        break;      
      case DELETE_ALL:
        type_str = METHOD_DELETE;
        HAL_Snprintf(pJsonDoc, sizeOfBuffer, "{\"Method\":\"%s\",\"State\":{\"Reported\":", type_str);
        break;      
      default:
        LOG_ERROR("unexpected method!");
        ret = ERR_PARAM_INVALID;
        break;
    }

    FUNC_EXIT_RC(ret);
}

/**
 * @brief 根据RequestParams来给json填入type字段
 */
static int shadow_json_set_content(char *pJsonDoc, size_t sizeOfBuffer, RequestParams *pParams)
{
    FUNC_ENTRY;

    POINTER_VALID_CHECK(pJsonDoc, ERR_PARAM_INVALID);
    
    int ret = SUCCESS_RET;
    int ret_of_snprintf = 0;
    size_t json_len = strlen(pJsonDoc);
    size_t remain_size = sizeOfBuffer - json_len;
    char delete_str[5] = "null";

    /* GET命令获取完整的影子文档,没有属性需要上报 */
    if(GET == pParams->method)
    {
        FUNC_EXIT_RC(ret);
    }

    if(DELETE_ALL == pParams->method)
    {
        ret_of_snprintf = HAL_Snprintf(pJsonDoc + json_len, remain_size, "%s",delete_str);
        ret = _check_snprintf_return(ret_of_snprintf, remain_size);
        FUNC_EXIT_RC(ret);
    }

    if (pParams->property_delta_list->len) 
    {
        ListIterator *iter;
        ListNode *node = NULL;
        DeviceProperty *DevProperty = NULL;

        if (NULL == (iter = list_iterator_new(pParams->property_delta_list, LIST_TAIL))) 
        {   
            ret = ERR_PARAM_INVALID;
            LOG_ERROR("property_delta_list is null\n");
            FUNC_EXIT_RC(ret);
        }

        for (;;) 
        {
            node = list_iterator_next(iter);
            if (NULL == node) {
                break;
            }

            DevProperty = (DeviceProperty *)(node->val);
            if (NULL == DevProperty) 
            {
                LOG_ERROR("node's value is invalid!\n");
                continue;
            }

            if (DevProperty->key != NULL) 
            {
                if((UPDATE == pParams->method) || (UPDATE_AND_RESET_VER == pParams->method) ||
                   (REPLY_CONTROL_UPDATE == pParams->method))
                {
                    put_json_node(pJsonDoc, remain_size, DevProperty->key, DevProperty->data, DevProperty->type);
                }
                else if((DELETE == pParams->method) || (REPLY_CONTROL_DELETE == pParams->method))
                {
                    put_json_node(pJsonDoc, remain_size, DevProperty->key, NULL, JSTRING);

                }
            }
            json_len = strlen(pJsonDoc);
            remain_size = sizeOfBuffer - json_len;

        }

        list_iterator_destroy(iter);
    }

    FUNC_EXIT_RC(ret);
}

/**
 * @brief 在JSON文档中添加结尾部分的内容, 包括version字段
 *
 */
static int shadow_json_finalize(UIoT_Shadow *pShadow, char *pJsonDoc, size_t sizeOfBuffer, RequestParams *pParams) 
{
    int ret;
    size_t remain_size = 0;
    int ret_of_snprintf = 0;

    if (pJsonDoc == NULL) 
    {
        return ERR_PARAM_INVALID;
    }

    if ((remain_size = sizeOfBuffer - strlen(pJsonDoc)) <= 1) 
    {
        return ERR_JSON_BUFFER_TOO_SMALL;
    }

    switch(pParams->method)
    {
        case GET:
            ret_of_snprintf = HAL_Snprintf(pJsonDoc + strlen(pJsonDoc), remain_size, "}");
            break;
        case DELETE_ALL:
            ret_of_snprintf = HAL_Snprintf(pJsonDoc + strlen(pJsonDoc), remain_size, "},\"Version\":%d}", pShadow->inner_data.version);
            break;
        case UPDATE_AND_RESET_VER:
            ret_of_snprintf = HAL_Snprintf(pJsonDoc + strlen(pJsonDoc) - 1, remain_size, "}},\"Version\":-1}");
            break;
        case REPLY_CONTROL_UPDATE:            
        case REPLY_CONTROL_DELETE:            
            if (pParams->property_delta_list->len) 
            {   
                ret_of_snprintf = HAL_Snprintf(pJsonDoc + strlen(pJsonDoc) - 1, remain_size, "},\"Desired\":null},\"Version\":%d}",pShadow->inner_data.version);
            }
            else
            {
                ret_of_snprintf = HAL_Snprintf(pJsonDoc + strlen(pJsonDoc), remain_size, "\"Desired\":null},\"Version\":%d}",pShadow->inner_data.version);
            }
            break;
        case UPDATE:
        case DELETE:
            ret_of_snprintf = HAL_Snprintf(pJsonDoc + strlen(pJsonDoc) - 1, remain_size, "}},\"Version\":%d}", pShadow->inner_data.version);
            break;
        default:
            ret= ERR_PARAM_INVALID;
            LOG_ERROR("undefined method!\n");
            break;
    }
    ret = _check_snprintf_return(ret_of_snprintf, remain_size);

    return ret;
}

/**
 * @brief 将设备影子文档的操作请求保存在列表中
 */
static int shadow_add_request_to_list(UIoT_Shadow *pShadow, RequestParams *pParams)
{
    FUNC_ENTRY;

    HAL_MutexLock(pShadow->request_mutex);
    if (pShadow->inner_data.request_list->len >= MAX_APPENDING_REQUEST_AT_ANY_GIVEN_TIME)
    {
        HAL_MutexUnlock(pShadow->request_mutex);
        FUNC_EXIT_RC(ERR_MAX_APPENDING_REQUEST);
    }
    
    Request *request = (Request *)HAL_Malloc(sizeof(Request));
    if (NULL == request) 
    {
        HAL_MutexUnlock(pShadow->request_mutex);
        LOG_ERROR("run memory malloc is error!\n");
        FUNC_EXIT_RC(FAILURE_RET);
    }
    
    request->callback = pParams->request_callback;
    request->property_delta_list = pParams->property_delta_list;
    request->user_context = pParams->user_context;
    request->method = pParams->method;
    
    init_timer(&(request->timer));
    countdown(&(request->timer), pParams->timeout_sec);
    
    ListNode *node = list_node_new(request);
    if (NULL == node) 
    {
        HAL_MutexUnlock(pShadow->request_mutex);
        LOG_ERROR("run list_node_new is error!\n");
        HAL_Free(request);
        FUNC_EXIT_RC(FAILURE_RET);
    }
    
    list_rpush(pShadow->inner_data.request_list, node);
    
    HAL_MutexUnlock(pShadow->request_mutex);
    
    FUNC_EXIT_RC(SUCCESS_RET);
}

#ifdef __cplusplus
}
#endif
