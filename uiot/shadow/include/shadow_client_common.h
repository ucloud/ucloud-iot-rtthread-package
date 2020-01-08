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


#ifndef IOT_SHADOW_CLIENT_COMMON_H_
#define IOT_SHADOW_CLIENT_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "shadow_client.h"

//设备影子相关topic
#define SHADOW_PUBLISH_REQUEST_TEMPLATE                 "/$system/%s/%s/shadow/upstream"
#define SHADOW_SUBSCRIBE_REQUEST_TEMPLATE               "/$system/%s/%s/shadow/downstream"
#define SHADOW_PUBLISH_SYNC_TEMPLATE                    "/$system/%s/%s/shadow/get"
#define SHADOW_SUBSCRIBE_SYNC_TEMPLATE                  "/$system/%s/%s/shadow/get_reply"
#define SHADOW_DOC_TEMPLATE                             "/$system/%s/%s/shadow/document"

/**
 * @brief 如果没有订阅delta主题, 则进行订阅, 并注册相应设备属性
 *
 * @param pShadow   shadow client
 * @param pProperty 设备属性
 * @param callback  相应设备属性处理回调函数
 * @return          返回SUCCESS, 表示成功
 */
int shadow_common_register_property_on_delta(UIoT_Shadow *pShadow, DeviceProperty *pProperty, OnPropRegCallback callback);

/**
 * @brief 更新属性值
 *
 * @param pShadow   shadow client
 * @param pProperty 设备属性
 * @return          返回SUCCESS, 表示成功
 */
int shadow_common_update_property(UIoT_Shadow *pshadow, DeviceProperty *pProperty, Method           method);

/**
 * @brief 移除注册过的设备属性
 *
 * @param pShadow   shadow client
 * @param pProperty 设备属性
 * @return          返回SUCCESS, 表示成功
 */
int shadow_common_remove_property(UIoT_Shadow *pshadow, DeviceProperty *pProperty);

/**
 * @brief 检查注册属性是否已经存在
 *
 * @param pShadow   shadow client
 * @param pProperty 设备属性
 * @return          返回 0, 表示属性不存在
 */
int shadow_common_check_property_existence(UIoT_Shadow *pshadow, DeviceProperty *pProperty);

/**
 * @brief 检查注册属性是否已经存在
 *
 * @param pParams   RequestParams
 * @param pProperty 设备属性
 * @return          返回 0, 表示修改属性添加成功
 */
int request_common_add_delta_property(RequestParams *pParams, DeviceProperty *pProperty);

/**
 * @brief 检查注册属性是否已经存在
 *
 * @param property_handle   PropertyHandler
 * @param pProperty         设备属性
 * @return                  返回 0, 表示匹配
 */
int shadow_common_check_property_match(void *property_handle, void *pProperty);

#ifdef __cplusplus
}
#endif

#endif //IOT_SHADOW_CLIENT_COMMON_H_
