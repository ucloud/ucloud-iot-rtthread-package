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

typedef enum {

    OTA_REPORT_UNDEFINED_ERROR = -6,
    OTA_REPORT_FIRMWARE_BURN_FAILED = -5,
    OTA_REPORT_MD5_MISMATCH = -4,
    OTA_REPORT_DOWNLOAD_TIMEOUT = -3,
    OTA_REPORT_SIGNATURE_EXPIRED = -2,
    OTA_REPORT_FIRMWARE_NOT_EXIST = -1,
    OTA_REPORT_NONE = 0,
    OTA_REPORT_DOWNLOADING = 1,
    OTA_REPORT_BURNING = 2,
    OTA_REPORT_SUCCESS = 3,
    OTA_REQUEST_FIRMWARE = 4,
    OTA_REPORT_VERSION = 5,

} IOT_OTA_UpstreamMsgType;

typedef int (*IOT_OTA_FetchCallback)(void *handle, IOT_OTA_UpstreamMsgType state);

/* OTA状态 */
typedef enum {

    OTA_STATE_UNINITED = 0,  /* 未初始化 */
    OTA_STATE_INITED,        /* 初始化完成 */
    OTA_STATE_FETCHING,      /* 正在下载固件 */
    OTA_STATE_FETCHED,       /* 固件下载完成 */
    OTA_STATE_DISCONNECTED   /* 连接已经断开 */

} IOT_OTA_State;

typedef struct  {
    uint32_t                id;                      /* message id */
    IOT_OTA_State           state;                   /* OTA state */
    uint32_t                size_last_fetched;       /* size of last downloaded */
    uint32_t                size_fetched;            /* size of already downloaded */
    uint32_t                size_file;               /* size of file */

    char                    *url;                    /* point to URL */
    char                    *download_name;          /* download partition name */
    char                    *module;                 /* download module name */
    char                    *version;                /* point to version */
    char                    *md5sum;                 /* MD5 string */

    void                    *md5;                    /* MD5 handle */
    void                    *ch_signal;              /* channel handle of signal exchanged with OTA server */
    void                    *ch_fetch;               /* channel handle of download */

    int                     err;                     /* last error code */

    Timer                   report_timer;
    IOT_OTA_FetchCallback   fetch_callback_func;
} OTA_Struct_t;

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
 * @param module:   版本所属类型 
 * @param version:  以字符串格式指定固件版本
 *
 * @retval > 0 : 对应publish的packet id
 * @retval < 0 : 失败，返回具体错误码
 */
int IOT_OTA_ReportVersion(void *handle, const char *module, const char *version);


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
 * @brief 检查固件是否已经下载完成
 *
 * @param handle: 指定OTA模块
 *
 * @retval 1 : Yes.
 * @retval 0 : No.
 */
int IOT_OTA_IsFetchFinish(void *handle);


/**
 * @brief 从具有特定超时值的远程服务器获取固件
 *        注意:如果你想要下载的更快，那么应该给出更大的“buf”
 *
 * @param handle:       指定OTA模块
 * @param buf:          指定存储固件数据的空间
 * @param buf_len:      用字节指定“buf”的长度
 * @param range_len:    用字节指定分片的长度
 * @param timeout_s:    超时时间
 *
 * @retval      < 0 : 对应的错误码
 * @retval        0 : 在“timeout_s”超时期间没有任何数据被下载
 * @retval (0, len] : 在“timeout_s”超时时间内以字节的方式下载数据的长度
 */
int IOT_OTA_FetchYield(void *handle, char *buf, size_t buf_len, size_t range_len, uint32_t timeout_s);


/**
 * @brief 获取指定的OTA信息
 *        通过这个接口，可以获得诸如状态、文件大小、文件的md5等信息
 *
 * @param handle:   指定OTA模块
 * @param type:     指定您想要的信息，请参见详细信息“IOT_OTA_CmdType”
 * @param buf:      为数据交换指定缓冲区
 * @param buf_len:  在字节中指定“buf”的长度
 * @return
      NOTE:
      1) 如果 type==OTA_IOCTL_FETCHED_SIZE, 'buf' 需要传入 uint32_t 类型指针, 'buf_len' 需指定为 4
      2) 如果 type==OTA_IOCTL_FILE_SIZE, 'buf' 需要传入 uint32_t 类型指针, 'buf_len' 需指定为 4
      3) 如果 type==OTA_IOCTL_MD5SUM, 'buf' 需要传入 buffer, 'buf_len' 需指定为 33
      4) 如果 type==OTA_IOCTL_VERSION, 'buf' 需要传入 buffer, 'buf_len' 需指定为 OTA_VERSION_LEN_MAX
      5) 如果 type==OTA_IOCTL_CHECK_FIRMWARE, 'buf' 需要传入 uint32_t 类型指针, 'buf_len'需指定为 4
         0, 固件MD5校验不通过, 固件是无效的; 1, 固件是有效的.
 *
 * @retval   0 : 执行成功
 * @retval < 0 : 执行失败，返回对应的错误码
 */
int IOT_OTA_Ioctl(void *handle, IOT_OTA_CmdType type, void *buf, size_t buf_len);


/**
 * @brief 得到最后一个错误代码
 *
 * @param handle: 指定OTA模块
 *
 * @return 对应错误的错误码.
 */
int IOT_OTA_GetLastError(void *handle);

/**
 * @brief 请求固件更新消息。设备离线时，不能接收服务端推送的升级消息。通过MQTT协议接入物联网平台的设备再次上线后，主动请求固件更新消息
 *
 * @param handle:   指定OTA模块
 * @param module:   版本所属类型 
 * @param version:  当前固件版本号
 *
 * @retval > 0 : 对应publish的packet id
 * @retval < 0 : 失败，返回具体错误码
 */
int IOT_OTA_RequestFirmware(void *handle, const char *module, const char *version);

/**
 * @brief 下载固件，下载结束后重启设备
 *
 * @param handle:   指定OTA模块
 *
 * @return 对应错误的错误码.
 */
int IOT_OTA_fw_download(void *handle);

#ifdef __cplusplus
}
#endif

#endif //C_SDK_UIOT_EXPORT_OTA_H_
