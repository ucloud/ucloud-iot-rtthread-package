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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "uiot_internal.h"
#include "shadow_client.h"
#include "lite-utils.h"
#include "shadow_client_common.h"

int IOT_Shadow_Request_Add_Delta_Property(void *handle, RequestParams *pParams, DeviceProperty *pProperty);

void* IOT_Shadow_Construct(const char *product_sn, const char *device_sn, void *ch_signal)
{
    FUNC_ENTRY;
    POINTER_VALID_CHECK(product_sn, NULL);
    POINTER_VALID_CHECK(device_sn, NULL);
    POINTER_VALID_CHECK(ch_signal, NULL);

    int ret;
    UIoT_Shadow *shadow_client = NULL;
    if ((shadow_client = (UIoT_Shadow *)HAL_Malloc(sizeof(UIoT_Shadow))) == NULL) {
        LOG_ERROR("memory not enough to malloc ShadowClient\n");
        return NULL;
    }

    UIoT_Client *mqtt_client = (UIoT_Client *)ch_signal;

    shadow_client->product_sn = product_sn;
    shadow_client->device_sn = device_sn;
    shadow_client->mqtt = mqtt_client;
    shadow_client->inner_data.version = 0;
    
    ret = uiot_shadow_init(shadow_client);    
    if (ret != SUCCESS_RET) {
        goto end;
    }

    /* 订阅更新影子文档必要的topic */
    ret = uiot_shadow_subscribe_topic(shadow_client, SHADOW_SUBSCRIBE_SYNC_TEMPLATE, topic_sync_handler);
    if (ret < 0)
    {
        LOG_ERROR("Subcribe %s fail!\n",SHADOW_SUBSCRIBE_SYNC_TEMPLATE);
        goto end;
    }
  
    ret = uiot_shadow_subscribe_topic(shadow_client, SHADOW_SUBSCRIBE_REQUEST_TEMPLATE, topic_request_result_handler);
    if (ret < 0)
    {
        LOG_ERROR("Subcribe %s fail!\n",SHADOW_SUBSCRIBE_SYNC_TEMPLATE);
        goto end;
    }

    return shadow_client;

end:
    HAL_Free(shadow_client);
    return NULL;
}

int IOT_Shadow_Destroy(void *handle)
{
    FUNC_ENTRY;
    POINTER_VALID_CHECK(handle, ERR_PARAM_INVALID);

    UIoT_Shadow* shadow_client = (UIoT_Shadow*)handle;
    uiot_shadow_reset(handle);

    if (NULL != shadow_client->request_mutex) {
        HAL_MutexDestroy(shadow_client->request_mutex);
    }

    if (NULL != shadow_client->property_mutex) {
        HAL_MutexDestroy(shadow_client->property_mutex);
    }

    HAL_Free(handle);

    FUNC_EXIT_RC(SUCCESS_RET);
}

int IOT_Shadow_Yield(void *handle, uint32_t timeout_ms) 
{
    FUNC_ENTRY;
    int ret;

    POINTER_VALID_CHECK(handle, ERR_PARAM_INVALID);
    NUMERIC_VALID_CHECK(timeout_ms, ERR_PARAM_INVALID);

    UIoT_Shadow *shadow_client = (UIoT_Shadow *)handle;

    _handle_expired_request(shadow_client);

    ret = IOT_MQTT_Yield(shadow_client->mqtt, timeout_ms);

    FUNC_EXIT_RC(ret);
}

int IOT_Shadow_Register_Property(void *handle, DeviceProperty *pProperty, OnPropRegCallback callback) 
{
    FUNC_ENTRY;
    POINTER_VALID_CHECK(handle, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pProperty, ERR_PARAM_INVALID);

    UIoT_Shadow* shadow_client = (UIoT_Shadow*)handle;

    if (IOT_MQTT_IsConnected(shadow_client->mqtt) == false) {
        FUNC_EXIT_RC(ERR_MQTT_NO_CONN);
    }

    if(shadow_client->inner_data.property_list->len)
    {
        if (shadow_common_check_property_existence(shadow_client, pProperty)) 
            FUNC_EXIT_RC(ERR_SHADOW_PROPERTY_EXIST);
    }
    
    int ret = shadow_common_register_property_on_delta(shadow_client, pProperty, callback);

    FUNC_EXIT_RC(ret);
}

int IOT_Shadow_UnRegister_Property(void *handle, DeviceProperty *pProperty)
{
    FUNC_ENTRY;

    POINTER_VALID_CHECK(handle, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pProperty, ERR_PARAM_INVALID);

    UIoT_Shadow* pshadow = (UIoT_Shadow*)handle;

    if (IOT_MQTT_IsConnected(pshadow->mqtt) == false) {
        FUNC_EXIT_RC(ERR_MQTT_NO_CONN);
    }

    if (!shadow_common_check_property_existence(pshadow, pProperty)) {
        FUNC_EXIT_RC(ERR_SHADOW_NOT_PROPERTY_EXIST);
    }

    int ret =  shadow_common_remove_property(pshadow, pProperty);

    FUNC_EXIT_RC(ret);
}


int IOT_Shadow_Get_Sync(void *handle, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context) 
{
    FUNC_ENTRY;
    int ret = SUCCESS_RET;

    POINTER_VALID_CHECK(handle, ERR_PARAM_INVALID);
    NUMERIC_VALID_CHECK(timeout_sec, ERR_PARAM_INVALID);

    UIoT_Shadow* shadow_client = (UIoT_Shadow*)handle;
    char JsonDoc[UIOT_MQTT_TX_BUF_LEN];
    size_t sizeOfBuffer = sizeof(JsonDoc) / sizeof(JsonDoc[0]);

    RequestParams *pParams = uiot_shadow_request_init(GET, request_callback, timeout_sec, user_context); 
        
    if (IOT_MQTT_IsConnected(shadow_client->mqtt) == false) 
    {
        FUNC_EXIT_RC(ERR_MQTT_NO_CONN);
    }

    ret = uiot_shadow_make_request(shadow_client, JsonDoc, sizeOfBuffer, pParams);
    if (ret != SUCCESS_RET) {
        FUNC_EXIT_RC(ret);
    }

    FUNC_EXIT_RC(ret);
}

int IOT_Shadow_Update(void *handle, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context, int property_count, ...) 
{
    FUNC_ENTRY;
    int ret = SUCCESS_RET;
    int loop = 0;

    POINTER_VALID_CHECK(handle, ERR_PARAM_INVALID);
    NUMERIC_VALID_CHECK(timeout_sec, ERR_PARAM_INVALID);

    UIoT_Shadow* shadow_client = (UIoT_Shadow*)handle;
    char JsonDoc[UIOT_MQTT_TX_BUF_LEN];
    size_t sizeOfBuffer = sizeof(JsonDoc) / sizeof(JsonDoc[0]);

    RequestParams *pParams = uiot_shadow_request_init(UPDATE, request_callback, timeout_sec, user_context); 

    va_list pArgs;
    va_start(pArgs, property_count);

    for(loop = 0; loop < property_count; loop++)
    {
        DeviceProperty *pProperty;
        pProperty  = va_arg(pArgs, DeviceProperty *);
        if(NULL != pProperty)
        {
            if(SUCCESS_RET != IOT_Shadow_Request_Add_Delta_Property(handle, pParams, pProperty))
            {
                va_end(pArgs);                
                return FAILURE_RET;
            }
         }
    }

    va_end(pArgs);
    
    if (IOT_MQTT_IsConnected(shadow_client->mqtt) == false) 
    {    
        FUNC_EXIT_RC(ERR_MQTT_NO_CONN);
    }

    ret = uiot_shadow_make_request(shadow_client, JsonDoc, sizeOfBuffer, pParams);
    if (ret != SUCCESS_RET) {
        FUNC_EXIT_RC(ret);
    }
    
    FUNC_EXIT_RC(ret);
}

int IOT_Shadow_Update_And_Reset_Version(void *handle, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context, int property_count, ...) 
{
    FUNC_ENTRY;
    int ret = SUCCESS_RET;
    int loop = 0;

    POINTER_VALID_CHECK(handle, ERR_PARAM_INVALID);
    NUMERIC_VALID_CHECK(timeout_sec, ERR_PARAM_INVALID);

    UIoT_Shadow* shadow_client = (UIoT_Shadow*)handle;
    char JsonDoc[UIOT_MQTT_TX_BUF_LEN];
    size_t sizeOfBuffer = sizeof(JsonDoc) / sizeof(JsonDoc[0]);

    RequestParams *pParams = uiot_shadow_request_init(UPDATE_AND_RESET_VER, request_callback, timeout_sec, user_context); 

    va_list pArgs;
    va_start(pArgs, property_count);

    for(loop = 0; loop < property_count; loop++)
    {
        DeviceProperty *pProperty;
        pProperty  = va_arg(pArgs, DeviceProperty *);
        if(NULL != pProperty)
        {
            if(SUCCESS_RET != IOT_Shadow_Request_Add_Delta_Property(handle, pParams, pProperty))
            {
                va_end(pArgs);
                return FAILURE_RET;
            }
         }
    }

    va_end(pArgs);
    
    if (IOT_MQTT_IsConnected(shadow_client->mqtt) == false) 
    {
        FUNC_EXIT_RC(ERR_MQTT_NO_CONN);
    }

    ret = uiot_shadow_make_request(shadow_client, JsonDoc, sizeOfBuffer, pParams);
    if (ret != SUCCESS_RET) {
        FUNC_EXIT_RC(ret);
    }

    FUNC_EXIT_RC(ret);
}

int IOT_Shadow_Delete(void *handle, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context, int property_count, ...) 
{
    FUNC_ENTRY;
    int ret = SUCCESS_RET;
    int loop = 0;

    POINTER_VALID_CHECK(handle, ERR_PARAM_INVALID);
    NUMERIC_VALID_CHECK(timeout_sec, ERR_PARAM_INVALID);

    UIoT_Shadow* shadow_client = (UIoT_Shadow*)handle;
    char JsonDoc[UIOT_MQTT_TX_BUF_LEN];
    size_t sizeOfBuffer = sizeof(JsonDoc) / sizeof(JsonDoc[0]);

    RequestParams *pParams = uiot_shadow_request_init(DELETE, request_callback, timeout_sec, user_context); 

    va_list pArgs;
    va_start(pArgs, property_count);

    for(loop = 0; loop < property_count; loop++)
    {
        DeviceProperty *pProperty;
        pProperty  = va_arg(pArgs, DeviceProperty *);
        if(NULL != pProperty)
        {
            if(SUCCESS_RET != IOT_Shadow_Request_Add_Delta_Property(handle, pParams, pProperty))
            {
                va_end(pArgs);
                return FAILURE_RET;
            }             
        }
    }

    va_end(pArgs);

    if (IOT_MQTT_IsConnected(shadow_client->mqtt) == false) 
    {
        FUNC_EXIT_RC(ERR_MQTT_NO_CONN);
    }

    ret = uiot_shadow_make_request(shadow_client, JsonDoc, sizeOfBuffer, pParams);
    if (ret != SUCCESS_RET) {
        FUNC_EXIT_RC(ret);
    }

    FUNC_EXIT_RC(ret);
}

int IOT_Shadow_Delete_All(void *handle, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context) 
{
    FUNC_ENTRY;
    int ret = SUCCESS_RET;

    POINTER_VALID_CHECK(handle, ERR_PARAM_INVALID);
    NUMERIC_VALID_CHECK(timeout_sec, ERR_PARAM_INVALID);

    UIoT_Shadow* shadow_client = (UIoT_Shadow*)handle;
    char JsonDoc[UIOT_MQTT_TX_BUF_LEN];
    size_t sizeOfBuffer = sizeof(JsonDoc) / sizeof(JsonDoc[0]);

    RequestParams *pParams = uiot_shadow_request_init(DELETE_ALL, request_callback, timeout_sec, user_context); 
     
    if (IOT_MQTT_IsConnected(shadow_client->mqtt) == false) 
    {
        FUNC_EXIT_RC(ERR_MQTT_NO_CONN);
    }

    ret = uiot_shadow_make_request(shadow_client, JsonDoc, sizeOfBuffer, pParams);
    if (ret != SUCCESS_RET) {
        FUNC_EXIT_RC(ret);
    }

    FUNC_EXIT_RC(ret);
}

int IOT_Shadow_Request_Add_Delta_Property(void *handle, RequestParams *pParams, DeviceProperty *pProperty)
{
    FUNC_ENTRY;
    int ret;

    POINTER_VALID_CHECK(pParams, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pProperty, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pProperty->key, ERR_PARAM_INVALID);
    
    ret = request_common_add_delta_property(pParams, pProperty);
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("request_common_add_delta_property fail\n");        
        FUNC_EXIT_RC(ret);
    }
        
    FUNC_EXIT_RC(ret);
}

int IOT_Shadow_Direct_Update_Value(char *value, DeviceProperty *pProperty) {
    FUNC_ENTRY;

    int ret = SUCCESS_RET;

    if (pProperty->type == JBOOL) {
        ret = LITE_get_boolean(pProperty->data, value);
    } else if (pProperty->type == JINT32) {
        ret = LITE_get_int32(pProperty->data, value);
    } else if (pProperty->type == JINT16) {
        ret = LITE_get_int16(pProperty->data, value);
    } else if (pProperty->type == JINT8) {
        ret = LITE_get_int8(pProperty->data, value);
    } else if (pProperty->type == JUINT32) {
        ret = LITE_get_uint32(pProperty->data, value);
    } else if (pProperty->type == JUINT16) {
        ret = LITE_get_uint16(pProperty->data, value);
    } else if (pProperty->type == JUINT8) {
        ret = LITE_get_uint8(pProperty->data, value);
    } else if (pProperty->type == JFLOAT) {
        ret = LITE_get_float(pProperty->data, value);
    } else if (pProperty->type == JDOUBLE) {
        ret = LITE_get_double(pProperty->data, value);
    }else if(pProperty->type == JSTRING){
        LOG_DEBUG("string type wait to be deal,%s\n",value);
    }else if(pProperty->type == JOBJECT){
        LOG_DEBUG("Json type wait to be deal,%s\n",value);
    }else{
        LOG_ERROR("pProperty type unknow,%d\n",pProperty->type);
    }

    FUNC_EXIT_RC(ret);
}

#ifdef __cplusplus
}
#endif

