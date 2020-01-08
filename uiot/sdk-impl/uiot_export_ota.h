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

#ifndef C_SDK_UIOT_EXPORT_OTA_H_
#define C_SDK_UIOT_EXPORT_OTA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "uiot_import.h"

typedef enum {

    OTA_IOCTL_FETCHED_SIZE,     /* 固件已经下载的大小 */
    OTA_IOCTL_FILE_SIZE,        /* 固件总大小 */
    OTA_IOCTL_MD5SUM,           /* md5(字符串类型) */
    OTA_IOCTL_VERSION,          /* 版本号(字符串类型)t */
    OTA_IOCTL_CHECK_FIRMWARE    /* 对固件进行校验 */

} IOT_OTA_CmdType;

typedef enum {

    OTA_PROGRESS_DOWNLOADING = 1,    /* 版本号(字符串类型)t */
    OTA_PROGRESS_BURNING = 2         /* 对固件进行校验 */

} IOT_OTA_ProgressState;

typedef enum {

    OTA_ERRCODE_FIRMWARE_NOT_EXIST = -1,      /* 固件文件不存在(URL无法访问) */
    OTA_ERRCODE_SIGNATURE_EXPIRED = -2,       /* URL签名过期 */
    OTA_ERRCODE_DOWNLOAD_TIMEOUT = -3,        /* 下载超时 */
    OTA_ERRCODE_MD5_MISMATCH = -4,            /* MD5不匹配 */
    OTA_ERRCODE_FIRMWARE_BURN_FAILED = -5,    /* 固件烧录失败 */
    OTA_ERRCODE_UNDEFINED_ERROR = -6          /* 未定义错误 */

} IOT_OTA_ReportErrCode;


/**
 * @brief 初始化OTA模块和返回句柄
 *        MQTT客户端必须在调用此接口之前进行初始化
 *
 * @param product_sn:   指定产品序列号
 * @param device_sn:    指定设备序列号
 * @param ch_signal:    指定的信号通道.
 *
 * @retval : 成功则返回句柄，失败返回NULL
 */
void *IOT_OTA_Init(const char *product_sn, const char *device_sn, void *ch_signal);


/**
 * @brief 释放OTA相关的资源
 *        如果在下载之后没有调用重新启动，则须调用该接口以释放资源
 *
 * @param handle: 指定OTA模块
 *
 * @retval   0 : 成功
 * @retval < 0 : 失败，返回具体错误码
 */
int IOT_OTA_Destroy(void *handle);


/**
 * @brief 向OTA服务器报告固件版本信息。
 *        NOTE: 进行OTA前请保证先上报一次本地固件的版本信息，以便服务器获取到设备目前的固件信息
 *
 * @param handle:   指定OTA模块
 * @param version:  以字符串格式指定固件版本
 *
 * @retval > 0 : 对应publish的packet id
 * @retval < 0 : 失败，返回具体错误码
 */
int IOT_OTA_ReportVersion(void *handle, const char *version);


/**
 * @brief 向OTA服务器报告详细进度
 *
 * @param handle:       指定OTA模块
 * @param progress:     下载或升级进度
 * @param reportType:   指定当前的OTA状态
 *
 * @retval 0 : 成功
 * @retval < 0 : 失败，返回具体错误码
 */
int IOT_OTA_ReportProgress(void *handle, int progress, IOT_OTA_ProgressState state);


/**
 * @brief 向OTA服务器上报升级成功
 *
 * @param handle:   指定OTA模块
 * @param version:  即将升级的固件信息
 *
 * @retval > 0 : 对应publish的packet id
 * @retval < 0 : 失败，返回具体错误码
 */
int IOT_OTA_ReportSuccess(void *handle, const char *version);


/**
 * @brief 向OTA服务器上报失败信息
 *
 * @param handle:    指定OTA模块
 * @param err_code:  错误码
 *
 * @retval > 0 : 对应publish的packet id
 * @retval < 0 : 失败，返回具体错误码
 */
int IOT_OTA_ReportFail(void *handle, IOT_OTA_ReportErrCode err_code);

/**
 * @brief 请求固件更新消息。设备离线时，不能接收服务端推送的升级消息。通过MQTT协议接入物联网平台的设备再次上线后，主动请求固件更新消息
 *
 * @param handle:   指定OTA模块
 * @param version:  当前固件版本号
 *
 * @retval > 0 : 对应publish的packet id
 * @retval < 0 : 失败，返回具体错误码
 */
int IOT_OTA_RequestFirmware(void *handle, const char *version);


#ifdef __cplusplus
}
#endif

#endif //C_SDK_UIOT_EXPORT_OTA_H_
