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


#ifndef C_SDK_UIOT_EXPORT_SHADOW_H_
#define C_SDK_UIOT_EXPORT_SHADOW_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "uiot_import.h"
#include "shadow_client.h"

/**
 * @brief 构造ShadowClient
 *
 * @param product_sn 产品序列号
 * @param device_sn 设备序列号
 * @param ch_signal 与MQTT服务器连接的句柄
 *
 * @return 返回NULL: 构造失败
 */
void* IOT_Shadow_Construct(const char *product_sn, const char *device_sn, void *ch_signal);

/**
 * @brief 销毁ShadowClient 关闭设备影子连接
 *
 * @param handle ShadowClient对象
 *
 * @return 返回SUCCESS, 表示成功
 */
int IOT_Shadow_Destroy(void *handle);

/**
 * @brief            消息接收, 心跳包管理, 超时请求处理
 *
 * @param handle     ShadowClient对象
 * @param timeout_ms 超时时间, 单位:ms
 * @return           返回SUCCESS, 表示调用成功
 */
int IOT_Shadow_Yield(void *handle, uint32_t timeout_ms);

/**
 * @brief 注册当前设备的设备属性
 *
 * @param pClient    ShadowClient对象
 * @param pProperty  设备属性
 * @param callback   设备属性更新回调处理函数
 * @return           返回SUCCESS, 表示请求成功
 */
int IOT_Shadow_Register_Property(void *handle, DeviceProperty *pProperty, OnPropRegCallback callback);

/**
 * @brief 注销当前设备的设备属性
 *
 * @param pClient    ShadowClient对象
 * @param pProperty  设备属性
 * @return           SUCCESS 请求成功
 */
int IOT_Shadow_UnRegister_Property(void *handle, DeviceProperty *pProperty);

/**
 * @brief 获取设备影子文档并同步设备离线期间设备影子更新的属性值和版本号
 *
 * @param pClient           ShadowClient对象
 * @param request_callback  请求回调函数
 * @param timeout_sec       请求超时时间, 单位:s
 * @param user_context      请求回调函数的用户数据
 * @return                  SUCCESS 请求成功
 */
int IOT_Shadow_Get_Sync(void *handle, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context); 

/**
 * @brief 设备更新设备影子的属性，变长入参的个数要和property_count的个数保持一致
 *
 * @param pClient           ShadowClient对象
 * @param request_callback  请求回调函数
 * @param timeout_sec       请求超时时间, 单位:s
 * @param user_context      请求回调函数的用户数据
 * @param property_count    变长入参的个数      
 * @param ...               变长入参设备的属性
 * @return                  SUCCESS 请求成功
 */
int IOT_Shadow_Update(void *handle, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context, int property_count, ...);

/**
 * @brief 更新属性并清零设备影子的版本号，变长入参的个数要和property_count的个数保持一致
 *
 * @param pClient           ShadowClient对象
 * @param request_callback  请求回调函数
 * @param timeout_sec       请求超时时间, 单位:s
 * @param user_context      请求回调函数的用户数据
 * @param property_count    变长入参的个数      
 * @param ...               变长入参设备的属性
 * @return                  SUCCESS 请求成功
 */
int IOT_Shadow_Update_And_Reset_Version(void *handle, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context, int property_count, ...); 

/**
 * @brief 设备删除设备影子的属性，变长入参的个数要和property_count的个数保持一致
 *
 * @param pClient           ShadowClient对象
 * @param request_callback  请求回调函数
 * @param timeout_sec       请求超时时间, 单位:s
 * @param user_context      请求回调函数的用户数据
 * @param property_count    变长入参的个数      
 * @param ...               变长入参设备的属性
 * @return                  SUCCESS 请求成功
 */
int IOT_Shadow_Delete(void *handle, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context, int property_count, ...); 

/**
 * @brief 设备删除全部设备影子的属性
 *
 * @param pClient           ShadowClient对象
 * @param request_callback  请求回调函数
 * @param timeout_sec       请求超时时间, 单位:s
 * @param user_context      请求回调函数的用户数据
 * @return                  SUCCESS 请求成功
 */
int IOT_Shadow_Delete_All(void *handle, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context);

/**
 * @brief 在请求中增加需要修改的属性
 *
 * @param pParams    设备影子文档修改请求
 * @param pProperty  设备属性
 * @return           返回SUCCESS, 表示请求成功
 */
int IOT_Shadow_Request_Add_Delta_Property(void *handle, RequestParams *pParams, DeviceProperty *pProperty);

/**
 * @brief 使用字符串的属性值直接更新属性值
 *
 * @param value         从云服务器设备影子文档Desired字段中解析出的字符串
 * @param pProperty     设备属性
 * @return              返回SUCCESS, 表示请求成功
 */
int IOT_Shadow_Direct_Update_Value(char *value, DeviceProperty *pProperty);

#ifdef __cplusplus
}
#endif

#endif /* C_SDK_UIOT_EXPORT_SHADOW_H_ */
