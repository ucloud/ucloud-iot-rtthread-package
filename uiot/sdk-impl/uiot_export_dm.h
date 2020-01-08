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

#ifndef C_SDK_UIOT_EXPORT_DM_H_
#define C_SDK_UIOT_EXPORT_DM_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "uiot_defs.h"

/* 设备物模型消息类型 */
typedef enum _dm_type {
    PROPERTY_RESTORE,         //设备恢复属性
    PROPERTY_POST,            //设备上报属性
    PROPERTY_SET,             //云端下发属性
    PROPERTY_DESIRED_GET,     //设备获取desire属性
    PROPERTY_DESIRED_DELETE,  //删除云端desire属性
    EVENT_POST,               //设备上报事件
    COMMAND,                  //命令下发
    DM_TYPE_MAX
}DM_Type;


typedef int (* PropertyRestoreCB)(const char *request_id, const int ret_code, const char *property);
typedef int (* PropertySetCB)(const char *request_id, const char *property);
typedef int (* PropertyDesiredGetCB)(const char *request_id, const int ret_code, const char *desired);
typedef int (* CommandCB)(const char *request_id, const char *identifier, const char *input, char **output);
typedef int (* CommonReplyCB)(const char *request_id, const int ret_code);

#define DECLARE_DM_CALLBACK(type, cb)  int uiot_register_for_##type(void*, cb);

DECLARE_DM_CALLBACK(PROPERTY_RESTORE,          PropertyRestoreCB)
DECLARE_DM_CALLBACK(PROPERTY_POST,             CommonReplyCB)
DECLARE_DM_CALLBACK(PROPERTY_SET,              PropertySetCB)
DECLARE_DM_CALLBACK(PROPERTY_DESIRED_GET,      PropertyDesiredGetCB)
DECLARE_DM_CALLBACK(PROPERTY_DESIRED_DELETE,   CommonReplyCB)
DECLARE_DM_CALLBACK(EVENT_POST,                CommonReplyCB)
DECLARE_DM_CALLBACK(COMMAND,                   CommandCB)

/**
 * @brief 注册消息回调函数的宏
 *
 * @param type:      消息类型，七种DM_Type之一
 * @param handle:    IOT_DM_Init返回的句柄
 * @param cb:        回调函数指针，函数类型必须与DECLARE_DM_CALLBACK声明的类型相同，
 *                   例如如果type是PROPERTY_RESTORE，cb则必须为PropertyRestoreCB类型
 *
 * @retval   0 : 成功
 * @retval < 0 : 失败，返回具体错误码
 */
#define IOT_DM_RegisterCallback(type, handle, cb)        uiot_register_for_##type(handle, cb);

/**
 * @brief 初始化dev_model模块和返回句柄
 *
 * @param product_sn:   指定产品序列号
 * @param device_sn:    指定设备序列号
 * @param ch_signal:    指定的信号通道.
 *
 * @retval 成功返回句柄，失败返回NULL.
 */
void *IOT_DM_Init(const char *product_sn, const char *device_sn, void *ch_signal);


/**
 * @brief 释放dev_model相关的资源
 *
 * @param handle: IOT_DM_Init返回的句柄
 *
 * @retval   0 : 成功
 * @retval < 0 : 失败，返回具体错误码
 */
int IOT_DM_Destroy(void *handle);


/**
 * @brief 属性有关的消息上报
 *
 * @param handle:     IOT_DM_Init返回的句柄
 * @param type:       消息类型，此处为
    PROPERTY_RESTORE,
    PROPERTY_POST,
    PROPERTY_DESIRED_GET,
    PROPERTY_DESIRED_DELETE
    四种消息类型之一
 * @param request_id: 消息的request_id
 * @param payload:    消息体
 *
 * @retval   0 : 成功
 * @retval < 0 : 失败，返回具体错误码
 */
int IOT_DM_Property_Report(void *handle, DM_Type type, int request_id, const char *payload);


/**
 * @brief 事件消息上报
 *
 * @param handle:     IOT_DM_Init返回的句柄
 * @param request_id: 消息的request_id
 * @param identifier: 事件标识符
 * @param payload:    事件Output消息体
 *
 * @retval   0 : 成功
 * @retval < 0 : 失败，返回具体错误码
 */
int IOT_DM_TriggerEvent(void *handle, int request_id, const char *identifier, const char *payload);


/**
 * @brief 在当前线程为底层MQTT客户端让出一定CPU执行时间，让其接收网络报文并将消息分发到用户的回调函数中
 *
 * @param handle:     IOT_DM_Init返回的句柄
 * @param timeout_ms: 超时时间，单位ms
 *
 * @retval   0 : 成功
 * @retval < 0 : 失败，返回具体错误码
 */
int IOT_DM_Yield(void *handle, uint32_t timeout_ms);

#if defined(__cplusplus)
}
#endif

#endif //C_SDK_UIOT_EXPORT_DM_H_
