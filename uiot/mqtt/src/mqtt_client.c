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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "mqtt_client.h"
#include "ca.h"

#include "uiot_import.h"
#include "uiot_export.h"
#include "utils_list.h"
#include "utils_net.h"


#define HOST_STR_LENGTH 64
static char s_uiot_host[HOST_STR_LENGTH] = {0};

#ifdef SUPPORT_TLS
static int s_uiot_port = 8883;
#else
static int s_uiot_port = 1883; 
#endif

void* IOT_MQTT_Construct(MQTTInitParams *pParams)
{
	POINTER_VALID_CHECK(pParams, NULL);
	STRING_PTR_VALID_CHECK(pParams->product_sn, NULL);
	STRING_PTR_VALID_CHECK(pParams->device_sn, NULL);
#ifndef AUTH_MODE_DYNAMIC
    STRING_PTR_VALID_CHECK(pParams->device_secret, NULL);
#endif

	UIoT_Client* mqtt_client = NULL;

	// 初始化MQTTClient
	if ((mqtt_client = (UIoT_Client*) HAL_Malloc (sizeof(UIoT_Client))) == NULL)
    {
		LOG_ERROR("malloc MQTTClient failed\n");
		return NULL;
	}
    
	int ret = uiot_mqtt_init(mqtt_client, pParams);
	if (ret != SUCCESS_RET) {
		LOG_ERROR("mqtt init failed: %d\n", ret);
        goto end;
    }

    MQTTConnectParams connect_params = DEFAULT_MQTT_CONNECT_PARAMS;
    int client_id_len = strlen(pParams->product_sn) + strlen(pParams->device_sn)+ 2;
	if((connect_params.client_id = (char*)HAL_Malloc(client_id_len)) == NULL)
    {
        LOG_ERROR("client_id init failed: %d\n", ret);
        goto end;
    }
    int username_len = strlen(pParams->product_sn) + strlen(pParams->device_sn) + 4;
    if((connect_params.username = (char*)HAL_Malloc(username_len)) == NULL)
    {
        LOG_ERROR("username init failed: %d\n", ret);
        HAL_Free(connect_params.client_id);
        goto end;
    }

    int auth_mode = 0;
#ifdef AUTH_MODE_DYNAMIC
    //动态认证时如果还未获取设备密钥则先使用产品密钥进行预认证
    if(NULL == pParams->device_secret)
    {
        auth_mode = DYNAMIC_AUTH;
        int password_len = strlen(pParams->product_secret) + 1;
        if((connect_params.password = (char*)HAL_Malloc(password_len)) == NULL)
        {
            LOG_ERROR("Dynamic password init failed: %d\n", ret);
            HAL_Free(connect_params.username);
            HAL_Free(connect_params.client_id);
            goto end;
        }
        HAL_Snprintf(connect_params.password, password_len, "%s", pParams->product_secret);
    }
#endif
    //使用获取到的设备密钥进行静态注册
    if(NULL != pParams->device_secret)
    {
        auth_mode = STATIC_AUTH;
        int password_len = strlen(pParams->device_secret) + 1;
        if((connect_params.password = (char*)HAL_Malloc(password_len)) == NULL)
        {
            LOG_ERROR("password init failed: %d\n", ret);
            HAL_Free(connect_params.username);
            HAL_Free(connect_params.client_id);
            goto end;
        }
        HAL_Snprintf(connect_params.password, password_len, "%s", pParams->device_secret); 
    }

    HAL_Snprintf(connect_params.client_id, client_id_len, "%s.%s", pParams->product_sn, pParams->device_sn);
    HAL_Snprintf(connect_params.username, username_len, "%s|%s|%d", pParams->product_sn, pParams->device_sn, auth_mode);
    connect_params.keep_alive_interval = pParams->keep_alive_interval;
	connect_params.clean_session = pParams->clean_session;
	connect_params.auto_connect_enable = pParams->auto_connect_enable;

	ret = uiot_mqtt_connect(mqtt_client, &connect_params);
	if (ret != SUCCESS_RET) {
		LOG_ERROR("mqtt connect failed: %d\n", ret);
        HAL_Free(connect_params.username);
        HAL_Free(connect_params.client_id);
        HAL_Free(connect_params.password);
        goto end;
	}
	else {
		LOG_INFO("mqtt connect success\n");
	}

	return mqtt_client;
end:
    HAL_Free(mqtt_client);
    return NULL;
}

int IOT_MQTT_Destroy(void **pClient) {
	POINTER_VALID_CHECK(*pClient, ERR_PARAM_INVALID);

	UIoT_Client *mqtt_client = (UIoT_Client *)(*pClient);

	int ret = uiot_mqtt_disconnect(mqtt_client);
    int loop = 0;
#ifdef MQTT_CHECK_REPEAT_MSG
    reset_repeat_packet_id_buffer();
#endif

    HAL_MutexDestroy(mqtt_client->lock_generic);
	HAL_MutexDestroy(mqtt_client->lock_write_buf);

    HAL_MutexDestroy(mqtt_client->lock_list_sub);
    HAL_MutexDestroy(mqtt_client->lock_list_pub);

    list_destroy(mqtt_client->list_pub_wait_ack);
    list_destroy(mqtt_client->list_sub_wait_ack);

    HAL_Free(mqtt_client->options.username);
    HAL_Free(mqtt_client->options.client_id);
    HAL_Free(mqtt_client->options.password);

    for (loop = 0; loop < MAX_SUB_TOPICS; ++loop)
    {
        HAL_Free((char *)mqtt_client->sub_handles[loop].topic_filter);
    }

    HAL_Free(*pClient);
    *pClient = NULL;

	return ret;
}

int IOT_MQTT_Yield(void *pClient, uint32_t timeout_ms) {

	UIoT_Client   *mqtt_client = (UIoT_Client *)pClient;
    int ret = uiot_mqtt_yield(mqtt_client, timeout_ms);

	return ret;
}

int IOT_MQTT_Publish(void *pClient, char *topicName, PublishParams *pParams) {

	UIoT_Client   *mqtt_client = (UIoT_Client *)pClient;

	return uiot_mqtt_publish(mqtt_client, topicName, pParams);
}

int IOT_MQTT_Subscribe(void *pClient, char *topicFilter, SubscribeParams *pParams) {

	UIoT_Client   *mqtt_client = (UIoT_Client *)pClient;

	return uiot_mqtt_subscribe(mqtt_client, topicFilter, pParams);
}

int IOT_MQTT_Unsubscribe(void *pClient, char *topicFilter) {

	UIoT_Client   *mqtt_client = (UIoT_Client *)pClient;

	return uiot_mqtt_unsubscribe(mqtt_client, topicFilter);
}

bool IOT_MQTT_IsConnected(void *pClient) {
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);

    UIoT_Client   *mqtt_client = (UIoT_Client *)pClient;

    return get_client_conn_state(mqtt_client) == 1;
}

static void on_message_callback_get_device_secret(void *pClient, MQTTMessage *message, void *userData) 
{    
    LOG_DEBUG("Receive Message With topicName:%.*s, payload:%.*s\n",
          (int) message->topic_len, message->topic, (int) message->payload_len, (char *) message->payload);
    uint32_t RetCode = 0;
    if(false == parse_mqtt_payload_retcode_type((char *)message->payload, &RetCode))
    {
        LOG_ERROR("parse retcode fail\n");
        return;
    }

    if(SUCCESS_RET != RetCode)
    {
        LOG_ERROR("get device secret fail\n");
        return;
    }
    
    char *Request_id = NULL;
    if(false == parse_mqtt_state_request_id_type((char *)message->payload, &Request_id))
    {
        LOG_ERROR("parse request_id fail\n");
        HAL_Free(Request_id);
        return;
    }

    if(0 != strcmp(Request_id, "1"))
    {
        LOG_ERROR("request_id error\n");
        HAL_Free(Request_id);
        return;
    }
    
    char *Password = NULL;
    if(false == parse_mqtt_state_password_type((char *)message->payload, &Password))
    {
        LOG_ERROR("parse password fail\n");
        HAL_Free(Request_id);
        HAL_Free(Password);
        return;
    }
    HAL_SetDeviceSecret(Password);
    HAL_Free(Request_id);
    HAL_Free(Password);    
    return;
}

int IOT_MQTT_Dynamic_Register(MQTTInitParams *pParams)
{   
    POINTER_VALID_CHECK(pParams, ERR_PARAM_INVALID);
    int ret = SUCCESS_RET;

    void *client = IOT_MQTT_Construct(pParams);
    if(NULL == client)
    {
        LOG_ERROR("mqtt client Dynamic register fail\n");
        return FAILURE_RET;
    }

    char *pub_name = (char *)HAL_Malloc(MAX_SIZE_OF_CLOUD_TOPIC * sizeof(char));
    if (pub_name == NULL) 
    {
        LOG_ERROR("topic_name malloc fail\n");
        IOT_MQTT_Destroy(&client);
        return FAILURE_RET;
    }
    memset(pub_name, 0x0, MAX_SIZE_OF_CLOUD_TOPIC);
	HAL_Snprintf(pub_name, MAX_SIZE_OF_CLOUD_TOPIC, DYNAMIC_REGISTER_PUB_TEMPLATE, pParams->product_sn, pParams->device_sn);

    /* 订阅回复设备密钥的topic */
    char *sub_name = (char *)HAL_Malloc(MAX_SIZE_OF_CLOUD_TOPIC * sizeof(char));
    if (sub_name == NULL) 
    {
        LOG_ERROR("topic_name malloc fail\n");
        HAL_Free(pub_name);
        IOT_MQTT_Destroy(&client);
        return FAILURE_RET;
    }
    memset(sub_name, 0x0, MAX_SIZE_OF_CLOUD_TOPIC);
	HAL_Snprintf(sub_name, MAX_SIZE_OF_CLOUD_TOPIC, DYNAMIC_REGISTER_SUB_TEMPLATE, pParams->product_sn, pParams->device_sn);

    SubscribeParams sub_params = DEFAULT_SUB_PARAMS;
    sub_params.qos = QOS1;
    sub_params.user_data = (void *)pParams->device_sn;
    sub_params.on_message_handler = on_message_callback_get_device_secret;
    ret = IOT_MQTT_Subscribe(client, sub_name, &sub_params);
    if(ret <= 0)
    {
        LOG_ERROR("subscribe %s fail\n",sub_name);
        goto end;
    }
    
    ret = IOT_MQTT_Yield(client,200);

    /* 向topic发送消息请求回复设备密钥 */
	char auth_msg[UIOT_MQTT_TX_BUF_LEN];
    HAL_Snprintf(auth_msg, UIOT_MQTT_TX_BUF_LEN, "{\"RequestID\":\"%d\"}", 1);
    PublishParams pub_params = DEFAULT_PUB_PARAMS;

    pub_params.qos = QOS1;
    pub_params.payload = (void *)auth_msg;
    pub_params.payload_len = strlen(auth_msg);
    
    IOT_MQTT_Publish(client, pub_name, &pub_params);

    ret = IOT_MQTT_Yield(client,5000);
    if (SUCCESS_RET != ret) 
    {
        LOG_ERROR("get device password fail\n");
        goto end;
    }
    
    IOT_MQTT_Unsubscribe(client, sub_name);
    ret = IOT_MQTT_Yield(client,200);
    if (SUCCESS_RET != ret) 
    {
        LOG_ERROR("unsub %s fail\n", sub_name);
        goto end;
    }
    
end:
    HAL_Free(pub_name);
    HAL_Free(sub_name);
    IOT_MQTT_Destroy(&client);
    return ret;
}

int uiot_mqtt_init(UIoT_Client *pClient, MQTTInitParams *pParams) {
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pParams, ERR_PARAM_INVALID);

	memset(pClient, 0x0, sizeof(UIoT_Client));

    int size = HAL_Snprintf(s_uiot_host, HOST_STR_LENGTH, "%s", UIOT_MQTT_DIRECT_DOMAIN);
    if (size < 0 || size > HOST_STR_LENGTH - 1) {
		return FAILURE_RET;
	}

    if (pParams->command_timeout < MIN_COMMAND_TIMEOUT)
    	pParams->command_timeout = MIN_COMMAND_TIMEOUT;
    if (pParams->command_timeout > MAX_COMMAND_TIMEOUT)
    	pParams->command_timeout = MAX_COMMAND_TIMEOUT;
    pClient->command_timeout_ms = pParams->command_timeout;

    // packet id 初始化时取1
    pClient->next_packet_id = 1;
    pClient->write_buf_size = UIOT_MQTT_TX_BUF_LEN;
    pClient->read_buf_size = UIOT_MQTT_RX_BUF_LEN;
    
    pClient->event_handler = pParams->event_handler;

    pClient->lock_generic = HAL_MutexCreate();
    if (NULL == pClient->lock_generic) {
        return FAILURE_RET;
    }

    set_client_conn_state(pClient, DISCONNECTED);

    if ((pClient->lock_write_buf = HAL_MutexCreate()) == NULL) {
    	LOG_ERROR("create write buf lock failed.");
    	goto error;
    }
    if ((pClient->lock_list_sub = HAL_MutexCreate()) == NULL) {
    	LOG_ERROR("create sub list lock failed.");
    	goto error;
    }
    if ((pClient->lock_list_pub = HAL_MutexCreate()) == NULL) {
    	LOG_ERROR("create pub list lock failed.");
    	goto error;
    }

    if ((pClient->list_pub_wait_ack = list_new()) == NULL) {
    	LOG_ERROR("create pub wait list failed.");
    	goto error;
    }
    pClient->list_pub_wait_ack->free = HAL_Free;

    if ((pClient->list_sub_wait_ack = list_new()) == NULL) {
    	LOG_ERROR("create sub wait list failed.");
        goto error;
    }
    pClient->list_sub_wait_ack->free = HAL_Free;

#ifdef SUPPORT_TLS
    // TLS连接参数初始化
    pClient->network_stack.authmode = SSL_CA_VERIFY_NONE;
    pClient->network_stack.ca_crt = iot_ca_get();
    pClient->network_stack.ca_crt_len = strlen(pClient->network_stack.ca_crt);
#endif
    pClient->network_stack.pHostAddress = s_uiot_host;
    pClient->network_stack.port = s_uiot_port;

    // 底层网络操作相关的数据结构初始化
    #ifdef SUPPORT_AT_CMD
    uiot_mqtt_network_init(&(pClient->network_stack), pClient->network_stack.pHostAddress,
            pClient->network_stack.port, pClient->network_stack.authmode, NULL);
    #else
    uiot_mqtt_network_init(&(pClient->network_stack), pClient->network_stack.pHostAddress,
            pClient->network_stack.port, pClient->network_stack.authmode, pClient->network_stack.ca_crt);
    #endif


    // ping定时器以及重连延迟定时器相关初始化
    init_timer(&(pClient->ping_timer));
    init_timer(&(pClient->reconnect_delay_timer));

    return SUCCESS_RET;

error:
	if (pClient->list_pub_wait_ack) {
		pClient->list_pub_wait_ack->free(pClient->list_pub_wait_ack);
		pClient->list_pub_wait_ack = NULL;
	}
	if (pClient->list_sub_wait_ack) {
		pClient->list_sub_wait_ack->free(pClient->list_sub_wait_ack);
		pClient->list_sub_wait_ack = NULL;
	}
	if (pClient->lock_generic) {
		HAL_MutexDestroy(pClient->lock_generic);
		pClient->lock_generic = NULL;
	}
	if (pClient->lock_list_sub) {
		HAL_MutexDestroy(pClient->lock_list_sub);
		pClient->lock_list_sub = NULL;
	}
	if (pClient->lock_list_pub) {
		HAL_MutexDestroy(pClient->lock_list_pub);
		pClient->lock_list_pub = NULL;
	}
	if (pClient->lock_write_buf) {
		HAL_MutexDestroy(pClient->lock_write_buf);
		pClient->lock_write_buf = NULL;
	}

	return FAILURE_RET;
}

int uiot_mqtt_set_autoreconnect(UIoT_Client *pClient, bool value) {
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);

    pClient->options.auto_connect_enable = (uint8_t) value;

    return SUCCESS_RET;
}

bool uiot_mqtt_is_autoreconnect_enabled(UIoT_Client *pClient) {
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);

    bool is_enabled = false;
    if (pClient->options.auto_connect_enable == 1) {
        is_enabled = true;
    }

    return is_enabled;
}

int uiot_mqtt_get_network_disconnected_count(UIoT_Client *pClient) {
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);

    return pClient->counter_network_disconnected;
}

int uiot_mqtt_reset_network_disconnected_count(UIoT_Client *pClient) {
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);

    pClient->counter_network_disconnected = 0;

    return SUCCESS_RET;
}

#ifdef __cplusplus
}
#endif
