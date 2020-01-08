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
#include "shadow_client_common.h"
#include "uiot_import.h"

/**
 * @brief 将注册属性的回调函数保存到列表之中
 */
static int _add_property_handle_to_list(UIoT_Shadow *pShadow, DeviceProperty *pProperty, OnPropRegCallback callback)
{
    FUNC_ENTRY;

    PropertyHandler *property_handle = (PropertyHandler *)HAL_Malloc(sizeof(PropertyHandler));
    if (NULL == property_handle)
    {
        LOG_ERROR("run memory malloc is error!\n");
        FUNC_EXIT_RC(FAILURE_RET);
    }

    property_handle->callback = callback;
    property_handle->property = pProperty;

    ListNode *node = list_node_new(property_handle);
    if (NULL == node) 
    {
        LOG_ERROR("run list_node_new is error!\n");
        FUNC_EXIT_RC(FAILURE_RET);
    }
    list_rpush(pShadow->inner_data.property_list, node);

    FUNC_EXIT_RC(SUCCESS_RET);
}

int shadow_common_check_property_match(void *pProperty, void *property_handle)
{
    FUNC_ENTRY;
    POINTER_VALID_CHECK(property_handle, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pProperty, ERR_PARAM_INVALID);

    int ret = SUCCESS_RET;
    PropertyHandler *property_handle_bak = property_handle;
    DeviceProperty *property_in_handle = (DeviceProperty *)property_handle_bak->property;
    DeviceProperty *property_bak = (DeviceProperty *)pProperty;
    
    LOG_DEBUG("property_in_handle->key:%s, property_bak->key:%s\n",property_in_handle->key,property_bak->key);
    
    if(!strcmp(property_in_handle->key, property_bak->key))
    {
        ret = true;
    }
    else
    {
        ret = false;
    }
    
    FUNC_EXIT_RC(ret);
}

int shadow_common_check_property_existence(UIoT_Shadow *pShadow, DeviceProperty *pProperty)
{
    FUNC_ENTRY;
    
    ListNode *node;

    HAL_MutexLock(pShadow->property_mutex);
    node = list_find(pShadow->inner_data.property_list, pProperty);
    HAL_MutexUnlock(pShadow->property_mutex);

    FUNC_EXIT_RC(NULL != node);
}

int shadow_common_update_property(UIoT_Shadow *pShadow, DeviceProperty *pProperty, Method           method)
{
    FUNC_ENTRY;

    int ret = SUCCESS_RET;
    ListNode *node;
    PropertyHandler *property_handle = NULL;
    DeviceProperty *property_bak =  NULL;
    
    HAL_MutexLock(pShadow->property_mutex);
    node = list_find(pShadow->inner_data.property_list, pProperty);
    if (NULL == node) 
    {
        ret = ERR_SHADOW_NOT_PROPERTY_EXIST;
        LOG_ERROR("Try to remove a non-existent property.\n");
    } 
    else 
    {
        property_handle = (PropertyHandler *)node->val;
        property_bak = (DeviceProperty *)property_handle->property;
        if((UPDATE == method) || (REPLY_CONTROL_UPDATE == method) || (UPDATE_AND_RESET_VER == method))
        {
            property_bak->data = pProperty->data;
        }
        else if((DELETE == method) || (REPLY_CONTROL_DELETE == method))
        {
            LOG_INFO("delete property\n");
            property_bak->data = NULL;
        }
        else
        {
            LOG_ERROR("error method\n");
            ret = ERR_PARAM_INVALID;
        }
    }
    
    HAL_MutexUnlock(pShadow->property_mutex);
    FUNC_EXIT_RC(ret);
}


int shadow_common_remove_property(UIoT_Shadow *pShadow, DeviceProperty *pProperty)
{
    FUNC_ENTRY;

    int ret = SUCCESS_RET;

    ListNode *node;
    HAL_MutexLock(pShadow->property_mutex);
    node = list_find(pShadow->inner_data.property_list, pProperty);
    if (NULL == node) 
    {
        ret = ERR_SHADOW_NOT_PROPERTY_EXIST;
        LOG_ERROR("Try to remove a non-existent property.\n");
    } 
    else 
    {
        list_remove(pShadow->inner_data.property_list, node);
    }
    HAL_MutexUnlock(pShadow->property_mutex);
    
    FUNC_EXIT_RC(ret);
}

int shadow_common_register_property_on_delta(UIoT_Shadow *pShadow, DeviceProperty *pProperty, OnPropRegCallback callback)
{
    FUNC_ENTRY;

    POINTER_VALID_CHECK(pShadow, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(callback, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pProperty, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pProperty->key, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pProperty->data, ERR_PARAM_INVALID);

    int ret;

    HAL_MutexLock(pShadow->property_mutex);
    ret = _add_property_handle_to_list(pShadow, pProperty, callback);
    HAL_MutexUnlock(pShadow->property_mutex);

    FUNC_EXIT_RC(ret);
}

int request_common_add_delta_property(RequestParams *pParams, DeviceProperty *pProperty)
{
    FUNC_ENTRY;

    POINTER_VALID_CHECK(pParams, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pProperty, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pProperty->key, ERR_PARAM_INVALID);

    int ret = SUCCESS_RET;
    
    ListNode *node = list_node_new(pProperty);
    if (NULL == node) 
    {
        LOG_ERROR("run list_node_new is error!\n");
        FUNC_EXIT_RC(FAILURE_RET);
    }
    list_rpush(pParams->property_delta_list, node);

    FUNC_EXIT_RC(ret);
}


#ifdef __cplusplus
}
#endif
