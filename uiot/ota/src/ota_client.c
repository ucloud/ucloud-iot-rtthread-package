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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uiot_defs.h"
#include "uiot_export_ota.h"
#include "uiot_internal.h"

#include "ota_config.h"
#include "ota_internal.h"

#include "utils_timer.h"

#include <fal.h>

#define HTTP_OTA_BUFF_LEN         4100

/* http slice len config HTTP_OTA_BUFF_LEN > HTTP_OTA_RANGE_LEN*/
#define HTTP_OTA_RANGE_LEN        4096

static void print_progress(uint32_t percent)
{
    static unsigned char progress_sign[100 + 1];
    uint8_t i;

    if (percent > 100)
    {
        percent = 100;
    }

    for (i = 0; i < 100; i++)
    {
        if (i < percent)
        {
            progress_sign[i] = '=';
        }
        else if (percent == i)
        {
            progress_sign[i] = '>';
        }
        else
        {
            progress_sign[i] = ' ';
        }
    }

    progress_sign[sizeof(progress_sign) - 1] = '\0';

    LOG_INFO("Download: [%s] %d%%\r\n", progress_sign, percent);
}

static void _ota_push_upload_msg(void *handle, const char *msg, uint32_t msg_len)
{
    OTA_Struct_t *h_ota = (OTA_Struct_t *) handle;
    OTA_MQTT_Struct_t *h_osc = h_ota->ch_signal;

    OTA_UPLOAD_Msg *push_msg = (OTA_UPLOAD_Msg *)HAL_Malloc(sizeof(OTA_UPLOAD_Msg));
    if (NULL == (push_msg->payload = HAL_Malloc(msg_len + 1))) {
        LOG_ERROR("HAL_Malloc failed!");
        return;
    }
    
    HAL_Snprintf(push_msg->payload, msg_len + 1, "%s", msg);
    push_msg->payload_len = msg_len;
    
    ListNode *node = list_node_new((void *)push_msg);
    if (NULL == node) 
    {
        LOG_ERROR("run list_node_new is error!\n");
        HAL_Free(push_msg->payload);
        HAL_Free(push_msg);
        return;
    }
    
    HAL_MutexLock(h_osc->msg_mutex);
    list_rpush(h_osc->msg_list, node);
    HAL_MutexUnlock(h_osc->msg_mutex);
    return;
}

static void _ota_pop_upload_msg(void *handle)
{
    OTA_Struct_t *h_ota = (OTA_Struct_t *) handle;
    OTA_MQTT_Struct_t *h_osc = h_ota->ch_signal;

    if(h_osc->msg_list->len > 0)
    {
        HAL_MutexLock(h_osc->msg_mutex);
        ListNode *node = list_lpop(h_osc->msg_list);
        HAL_MutexUnlock(h_osc->msg_mutex);            
        OTA_UPLOAD_Msg *pop_msg = node->val;
        h_osc->msg_callback(h_ota, pop_msg->payload, pop_msg->payload_len);
        HAL_Free(pop_msg->payload);
        HAL_Free(pop_msg);
        HAL_Free(node);
        return;
    }
    return;
}

static void _ota_callback(void *pContext, const char *msg, uint32_t msg_len) 
{
    char *msg_method = NULL;
    char *msg_str = NULL;
    char *msg_module = NULL;        
    char *msg_ver = NULL;
    
    OTA_Struct_t *h_ota = (OTA_Struct_t *) pContext;

    if (h_ota == NULL || msg == NULL) {
        LOG_ERROR("pointer is NULL");
        return;
    }

    if (NULL == (msg_str = HAL_Malloc(msg_len + 1))) {
        LOG_ERROR("HAL_Malloc failed!");
        return;
    }

    HAL_Snprintf(msg_str, msg_len + 1, "%s", msg);

    if (SUCCESS_RET != ota_lib_get_msg_type(msg_str, &msg_method)) {
        LOG_ERROR("Get message type failed!");
        goto do_exit;
    }

    if((NULL == h_ota->ch_fetch) && (0 == strcmp(msg_method, CANCEL_UPDATE_METHOD)))
    {    
        LOG_ERROR("download is canceled!");
        goto do_exit;
    }

    if (0 == strcmp(msg_method, UPDATE_FIRMWARE_METHOD)) 
    {    
        /* downloading, push update msg to list */
        if (h_ota->state == OTA_STATE_FETCHING) {                 
            LOG_INFO("In OTA_STATE_FETCHING state");
            _ota_push_upload_msg(h_ota, msg_str, msg_len);
            goto do_exit;
        }
                
        if (SUCCESS_RET != ota_lib_get_params(msg_str, &h_ota->url, &h_ota->module, &h_ota->download_name, &h_ota->version, &h_ota->md5sum, &h_ota->size_file)) {
            LOG_ERROR("Get firmware parameter failed");
            goto do_exit;
        }

        if (NULL == (h_ota->ch_fetch = ofc_init(h_ota->url))) {
            LOG_ERROR("Initialize fetch module failed");
            goto do_exit;
        }

        if (SUCCESS_RET != ofc_connect(h_ota->ch_fetch)) {
            LOG_ERROR("Connect fetch module failed");
            h_ota->state = OTA_STATE_DISCONNECTED;
            goto do_exit;
        }

        h_ota->state = OTA_STATE_FETCHING;
        
        if(SUCCESS_RET != IOT_OTA_fw_download(h_ota)) {
            LOG_ERROR("download file failed");
            h_ota->state = OTA_STATE_DISCONNECTED;
        }

        /* download over, pop first pushed msg to download */
        _ota_pop_upload_msg(h_ota);
        
    }
    else if(0 == strcmp(msg_method, CANCEL_UPDATE_METHOD))
    {            
        if (SUCCESS_RET != ota_lib_get_msg_module_ver(msg_str, &msg_module, &msg_ver)) {
            LOG_ERROR("Get message module failed!");               
            HAL_Free(msg_module);        
            HAL_Free(msg_ver);
            goto do_exit;
        }

        if((0 == strcmp(msg_module, h_ota->module)) && (0 == strcmp(msg_ver, h_ota->version)))
        {                   
            OTA_Http_Client *h_ofc = (OTA_Http_Client *)h_ota->ch_fetch;
            http_client_close(&h_ofc->http);
            h_ota->state = OTA_STATE_DISCONNECTED;
        }
        HAL_Free(msg_module);        
        HAL_Free(msg_ver);
    }
do_exit:
    HAL_Free(msg_str);
    HAL_Free(msg_method);
    return;
}


int IOT_OTA_ReportProgress(void *handle, int progress, IOT_OTA_ProgressState state)
{
    POINTER_VALID_CHECK(handle, ERR_OTA_INVALID_PARAM);

    int ret;
    char *msg_report;
    OTA_Struct_t *h_ota = (OTA_Struct_t *) handle;

    if (OTA_STATE_UNINITED == h_ota->state) {
        LOG_ERROR("handle is uninitialized");
        h_ota->err = ERR_OTA_INVALID_STATE;
        return ERR_OTA_INVALID_STATE;
    }

    if (progress < 0 || progress > 100) {
        LOG_ERROR("progress is a invalid parameter");
        h_ota->err = ERR_OTA_INVALID_PARAM;
        return ERR_OTA_INVALID_PARAM;
    }

    if (NULL == (msg_report = HAL_Malloc(OTA_UPSTREAM_MSG_BUF_LEN))) {
        LOG_ERROR("allocate memory for msg_report failed");
        h_ota->err = ERR_OTA_NO_MEMORY;
        return ERR_OTA_NO_MEMORY;
    }

    ret = ota_lib_gen_upstream_msg(msg_report, OTA_UPSTREAM_MSG_BUF_LEN, h_ota->module, h_ota->version, progress, (IOT_OTA_UpstreamMsgType)state);
    if (SUCCESS_RET != ret) {
        LOG_ERROR("generate reported message failed");
        h_ota->err = ret;
        goto do_exit;
    }

    ret = osc_report_progress(h_ota->ch_signal, msg_report);
    if (ret < 0) {
        LOG_ERROR("Report progress failed");
        h_ota->err = ret;
        goto do_exit;
    }

do_exit:
    if (NULL != msg_report) {
        HAL_Free(msg_report);
    }
    return ret;
}


static int send_upstream_msg_with_version(void *handle, const char *module, const char *version, IOT_OTA_UpstreamMsgType reportType)
{
    POINTER_VALID_CHECK(handle, ERR_OTA_INVALID_PARAM);
    POINTER_VALID_CHECK(version, ERR_OTA_INVALID_PARAM);

    int ret, len;
    char *msg_upstream;
    OTA_Struct_t *h_ota = (OTA_Struct_t *)handle;

    if (OTA_STATE_UNINITED == h_ota->state) {
        LOG_ERROR("handle is uninitialized");
        h_ota->err = ERR_OTA_INVALID_STATE;
        return ERR_OTA_INVALID_STATE;
    }

    len = strlen(version);
    if ((len < OTA_VERSION_STR_LEN_MIN) || (len > OTA_VERSION_STR_LEN_MAX)) {
        LOG_ERROR("version string is invalid: must be [1, 32] chars");
        h_ota->err = ERR_OTA_INVALID_PARAM;
        return ERR_OTA_INVALID_PARAM;
    }

    if (NULL == (msg_upstream = HAL_Malloc(OTA_UPSTREAM_MSG_BUF_LEN))) {
        LOG_ERROR("allocate for msg_informed failed");
        h_ota->err = ERR_OTA_NO_MEMORY;
        return ERR_OTA_NO_MEMORY;
    }

    ret = ota_lib_gen_upstream_msg(msg_upstream, OTA_UPSTREAM_MSG_BUF_LEN, module, version, 0, reportType);
    if (SUCCESS_RET != ret) {
        LOG_ERROR("generate upstream message failed");
        h_ota->err = ret;
        goto do_exit;
    }

    ret = osc_upstream_publish(h_ota->ch_signal, msg_upstream);
    if (ret < 0) {
        LOG_ERROR("Report result failed");
        h_ota->err = ret;
        goto do_exit;
    }

do_exit:
    if (NULL != msg_upstream) {
        HAL_Free(msg_upstream);
    }
    return ret;
}

void *IOT_OTA_Init(const char *product_sn, const char *device_sn, void *ch_signal)
{
    POINTER_VALID_CHECK(product_sn, NULL);
    POINTER_VALID_CHECK(device_sn, NULL);
    POINTER_VALID_CHECK(ch_signal, NULL);

    OTA_Struct_t *h_ota = NULL;

    if (NULL == (h_ota = HAL_Malloc(sizeof(OTA_Struct_t)))) {
        LOG_ERROR("allocate failed");
        return NULL;
    }
    memset(h_ota, 0, sizeof(OTA_Struct_t));
    h_ota->state = OTA_STATE_UNINITED;

    h_ota->ch_signal = osc_init(product_sn, device_sn, ch_signal, _ota_callback, h_ota);
    if (NULL == h_ota->ch_signal) {
        LOG_ERROR("initialize signal channel failed");
        goto do_exit;
    }
    
    h_ota->md5 = ota_lib_md5_init();
    if (NULL == h_ota->md5) {
        LOG_ERROR("initialize md5 failed");
        goto do_exit;
    }

    h_ota->state = OTA_STATE_INITED;
    return h_ota;

do_exit:
    if (NULL != h_ota->ch_signal) {
        osc_deinit(h_ota->ch_signal);
    }

    if (NULL != h_ota->md5) {
        ota_lib_md5_deinit(h_ota->md5);
    }

    if (NULL != h_ota) {
        HAL_Free(h_ota);
    }

    return NULL;
}


int IOT_OTA_Destroy(void *handle)
{
    POINTER_VALID_CHECK(handle, ERR_OTA_INVALID_PARAM);

    OTA_Struct_t *h_ota = (OTA_Struct_t*) handle;

    if (OTA_STATE_UNINITED == h_ota->state) {
        LOG_ERROR("handle is uninitialized");
        return FAILURE_RET;
    }

    osc_deinit(h_ota->ch_signal);    
    ofc_deinit(h_ota->ch_fetch);
    ota_lib_md5_deinit(h_ota->md5);

    if (NULL != h_ota->url) {
        HAL_Free(h_ota->url);
    }

    if (NULL != h_ota->module) {
        HAL_Free(h_ota->module);
    }

    if (NULL != h_ota->version) {
        HAL_Free(h_ota->version);
    }

    if (NULL != h_ota->md5sum) {
        HAL_Free(h_ota->md5sum);
    }

    if (NULL != h_ota->download_name) {
        HAL_Free(h_ota->download_name);
    }

    HAL_Free(h_ota);
    return SUCCESS_RET;
}

void IOT_OTA_Clear(void *handle)
{
    OTA_Struct_t *h_ota = (OTA_Struct_t *)handle;
    
    ofc_deinit(h_ota->ch_fetch);
    
    if (NULL != h_ota->url) {
        memset(h_ota->url, 0, strlen(h_ota->url));
    }

    if (NULL != h_ota->module) {
        memset(h_ota->module, 0, strlen(h_ota->module));
    }
    
    if(NULL != h_ota->download_name){
        memset(h_ota->download_name, 0, strlen(h_ota->download_name));
    }
    
    if (NULL != h_ota->version) {
        memset(h_ota->version, 0, strlen(h_ota->version));    
    }

    if (NULL != h_ota->md5sum) {
        memset(h_ota->md5sum, 0, strlen(h_ota->md5sum));
    }
    
    h_ota->size_last_fetched = 0;
    h_ota->size_fetched = 0;
    h_ota->size_file = 0;    
    ota_lib_md5_deinit(h_ota->md5);   
    h_ota->md5 = ota_lib_md5_init();    
    h_ota->state = OTA_STATE_INITED;
    return;
}

int IOT_OTA_ReportVersion(void *handle, const char *module, const char *version)
{
    return send_upstream_msg_with_version(handle, module, version, OTA_REPORT_VERSION);
}


int IOT_OTA_RequestFirmware(void *handle, const char *module, const char *version)
{
    return send_upstream_msg_with_version(handle, module, version, OTA_REQUEST_FIRMWARE);
}

int IOT_OTA_ReportSuccess(void *handle, const char *version)
{   
    OTA_Struct_t *h_ota = (OTA_Struct_t *)handle;
    
    if(h_ota->fetch_callback_func != NULL)
    {
        h_ota->fetch_callback_func(handle, OTA_REPORT_SUCCESS);
    }
    return send_upstream_msg_with_version(handle, h_ota->module, version, OTA_REPORT_SUCCESS);
}

int IOT_OTA_ReportFail(void *handle, IOT_OTA_ReportErrCode err_code)
{
    POINTER_VALID_CHECK(handle, ERR_OTA_INVALID_PARAM);

    int ret;
    char *msg_upstream;
    OTA_Struct_t *h_ota = (OTA_Struct_t *)handle;

    if (OTA_STATE_UNINITED == h_ota->state) {
        LOG_ERROR("handle is uninitialized");
        h_ota->err = ERR_OTA_INVALID_STATE;
        return ERR_OTA_INVALID_STATE;
    }

    if (NULL == (msg_upstream = HAL_Malloc(OTA_UPSTREAM_MSG_BUF_LEN))) {
        LOG_ERROR("allocate for msg_informed failed");
        h_ota->err = ERR_OTA_NO_MEMORY;
        return ERR_OTA_NO_MEMORY;
    }

    ret = ota_lib_gen_upstream_msg(msg_upstream, OTA_UPSTREAM_MSG_BUF_LEN, h_ota->module, "", 0, (IOT_OTA_UpstreamMsgType)err_code);
    if (SUCCESS_RET != ret) {
        LOG_ERROR("generate upstream message failed");
        h_ota->err = ret;
        goto do_exit;
    }

    ret = osc_upstream_publish(h_ota->ch_signal, msg_upstream);
    if (ret < 0) {
        LOG_ERROR("Report result failed");
        h_ota->err = ret;
        goto do_exit;
    }

    if(h_ota->fetch_callback_func != NULL)
        h_ota->fetch_callback_func(handle, (IOT_OTA_UpstreamMsgType)err_code);

    do_exit:
    if (NULL != msg_upstream) {
        HAL_Free(msg_upstream);
    }
    return ret;
}


int IOT_OTA_IsFetching(void *handle)
{
    OTA_Struct_t *h_ota = (OTA_Struct_t *)handle;

    if (NULL == handle) {
        LOG_ERROR("handle is NULL");
        return 0;
    }

    if (OTA_STATE_UNINITED == h_ota->state) {
        LOG_ERROR("handle is uninitialized");
        h_ota->err = ERR_OTA_INVALID_STATE;
        return 0;
    }

    return (OTA_STATE_FETCHING == h_ota->state);
}


int IOT_OTA_IsFetchFinish(void *handle)
{
    OTA_Struct_t *h_ota = (OTA_Struct_t *) handle;

    if (NULL == handle) {
        LOG_ERROR("handle is NULL");
        return 0;
    }

    if (OTA_STATE_UNINITED == h_ota->state) {
        LOG_ERROR("handle is uninitialized");
        h_ota->err = ERR_OTA_INVALID_STATE;
        return 0;
    }

    return (OTA_STATE_FETCHED == h_ota->state);
}

int IOT_OTA_Yield(void *handle, uint32_t timeout_ms)
{
    POINTER_VALID_CHECK(handle, FAILURE_RET);

    OTA_Struct_t *h_ota = (OTA_Struct_t*) handle;

    return IOT_MQTT_Yield(((OTA_MQTT_Struct_t *)h_ota->ch_signal)->mqtt, timeout_ms);
}

int IOT_OTA_FetchYield(void *handle, char *buf, size_t buf_len, size_t range_len, uint32_t timeout_s)
{
    int ret;
    OTA_Struct_t *h_ota = (OTA_Struct_t *) handle;
    int retry_time = 0;

    POINTER_VALID_CHECK(handle, ERR_OTA_INVALID_PARAM);
    POINTER_VALID_CHECK(buf, ERR_OTA_INVALID_PARAM);
    NUMERIC_VALID_CHECK(buf_len, ERR_OTA_INVALID_PARAM);

    if (OTA_STATE_FETCHING != h_ota->state) {
        h_ota->err = ERR_OTA_INVALID_STATE;
        return ERR_OTA_INVALID_STATE;
    }

    for(retry_time = 0; retry_time < 5; retry_time++)
    {
        /* fetch fail,try again utill 5 time */
        ret = ofc_fetch(h_ota->ch_fetch, h_ota->size_fetched ,buf, buf_len, range_len, timeout_s);
        /* range download send request too often maybe cutdown by server, need reconnect and continue to download. */
        if((ret == ERR_HTTP_CONN_ERROR) && (h_ota->state != OTA_STATE_DISCONNECTED)) {
            ofc_deinit(h_ota->ch_fetch);
            h_ota->ch_fetch = ofc_init(h_ota->url);
            ofc_connect(h_ota->ch_fetch);            
            h_ota->state = OTA_STATE_FETCHING;
            continue;
        } 
        if (ret < 0) {
            LOG_ERROR("Fetch firmware failed");
            h_ota->state = OTA_STATE_FETCHED;
            h_ota->err = ret;

            if (ret == ERR_OTA_FETCH_AUTH_FAIL) { // 上报签名过期
                IOT_OTA_ReportFail(h_ota, OTA_ERRCODE_SIGNATURE_EXPIRED);
            } else if (ret == ERR_OTA_FILE_NOT_EXIST) { // 上报文件不存在
                IOT_OTA_ReportFail(h_ota, OTA_ERRCODE_FIRMWARE_NOT_EXIST);
            } else if (ret == ERR_OTA_FETCH_TIMEOUT) { // 上报下载超时
                IOT_OTA_ReportFail(h_ota, OTA_ERRCODE_DOWNLOAD_TIMEOUT);
            } else {
                h_ota->err = ERR_OTA_FETCH_FAILED;
            }
            HAL_SleepMs(1000);
        } else if (0 == h_ota->size_fetched) {
            /* force report status in the first */
            IOT_OTA_ReportProgress(h_ota, 0, OTA_PROGRESS_DOWNLOADING);

            init_timer(&h_ota->report_timer);
            countdown(&h_ota->report_timer, OTA_REPORT_PROGRESS_INTERVAL);
            break;
        } else {
            break;
        }
    }
    if (ret > 0) {
        ota_lib_md5_update(h_ota->md5, buf, ret);        
        h_ota->size_last_fetched = ret;
        h_ota->size_fetched += ret;
    }
    else
    {
        return ret;
    }

    /* report percent every second. */
    uint32_t percent = (h_ota->size_fetched * 100) / h_ota->size_file;
    if (percent == 100) {
        IOT_OTA_ReportProgress(h_ota, percent, OTA_PROGRESS_DOWNLOADING);
    } else if (h_ota->size_last_fetched > 0 && has_expired(&h_ota->report_timer)) {
        IOT_OTA_ReportProgress(h_ota, percent, OTA_PROGRESS_DOWNLOADING);
        countdown(&h_ota->report_timer, OTA_REPORT_PROGRESS_INTERVAL);
        HAL_SleepMs(100);
    }

    print_progress(percent);

    if (h_ota->size_fetched >= h_ota->size_file) {
        h_ota->state = OTA_STATE_FETCHED;
    }

    return ret;
}


int IOT_OTA_Ioctl(void *handle, IOT_OTA_CmdType type, void *buf, size_t buf_len)
{
    OTA_Struct_t * h_ota = (OTA_Struct_t *) handle;

    POINTER_VALID_CHECK(handle, ERR_OTA_INVALID_PARAM);
    POINTER_VALID_CHECK(buf, ERR_OTA_INVALID_PARAM);
    NUMERIC_VALID_CHECK(buf_len, ERR_OTA_INVALID_PARAM);

    if (h_ota->state < OTA_STATE_FETCHING) {
        h_ota->err = ERR_OTA_INVALID_STATE;
        return ERR_OTA_INVALID_STATE;
    }

    switch (type) {
        case OTA_IOCTL_FETCHED_SIZE:
            if ((4 != buf_len) || (0 != ((unsigned long)buf & 0x3))) {
                LOG_ERROR("Invalid parameter");
                h_ota->err = ERR_OTA_INVALID_PARAM;
                return FAILURE_RET;
            } else {
                *((uint32_t *)buf) = h_ota->size_fetched;
                return SUCCESS_RET;
            }

        case OTA_IOCTL_FILE_SIZE:
            if ((4 != buf_len) || (0 != ((unsigned long)buf & 0x3))) {
                LOG_ERROR("Invalid parameter");
                h_ota->err = ERR_OTA_INVALID_PARAM;
                return FAILURE_RET;
            } else {
                *((uint32_t *)buf) = h_ota->size_file;
                return SUCCESS_RET;
            }

        case OTA_IOCTL_VERSION:
            strncpy(buf, h_ota->version, buf_len);
            ((char *)buf)[buf_len - 1] = '\0';
            break;

        case OTA_IOCTL_MD5SUM:
            strncpy(buf, h_ota->md5sum, buf_len);
            ((char *)buf)[buf_len - 1] = '\0';
            break;

        case OTA_IOCTL_CHECK_FIRMWARE:
            if ((4 != buf_len) || (0 != ((unsigned long)buf & 0x3))) {
                LOG_ERROR("Invalid parameter");
                h_ota->err = ERR_OTA_INVALID_PARAM;
                return FAILURE_RET;
            } else if (h_ota->state != OTA_STATE_FETCHED) {
                h_ota->err = ERR_OTA_INVALID_STATE;
                LOG_ERROR("Firmware can be checked in OTA_STATE_FETCHED state only");
                return FAILURE_RET;
            } else {
                char md5_str[33];
                ota_lib_md5_finalize(h_ota->md5, md5_str);
                LOG_DEBUG("origin=%s, now=%s", h_ota->md5sum, md5_str);
                if (0 == strcmp(h_ota->md5sum, md5_str)) {
                    *((uint32_t *)buf) = 1;
                } else {
                    *((uint32_t *)buf) = 0;
                    // 上报MD5不匹配
                    h_ota->err = ERR_OTA_MD5_MISMATCH;
                    IOT_OTA_ReportFail(h_ota, OTA_ERRCODE_MD5_MISMATCH);
                }
                return SUCCESS_RET;
            }
            
        default:
            LOG_ERROR("invalid cmd type");
            h_ota->err = ERR_OTA_INVALID_PARAM;
            return FAILURE_RET;
    }

    return SUCCESS_RET;
}


int IOT_OTA_GetLastError(void *handle)
{
    OTA_Struct_t * h_ota = (OTA_Struct_t *) handle;

    if (NULL == handle) {
        LOG_ERROR("handle is NULL");
        return  ERR_OTA_INVALID_PARAM;
    }

    return h_ota->err;
}

int IOT_OTA_fw_download(void *handle)
{
    int ret = 0;
    int file_size = 0, length, firmware_valid, total_length = 0;
    char *buffer_read = NULL;
    OTA_Struct_t * h_ota = (OTA_Struct_t *) handle;
    void * download_handle = NULL;
    // 用于存放云端下发的固件版本
    char msg_version[33];

    IOT_OTA_Ioctl(h_ota, OTA_IOCTL_FILE_SIZE, &file_size, 4);

    download_handle = HAL_Download_Init(h_ota->download_name);
    if(download_handle == NULL)
    {
        ret = FAILURE_RET;
        goto __exit;
    }

    buffer_read = (char *)HAL_Malloc(HTTP_OTA_BUFF_LEN);
    if (buffer_read == NULL)
    {
        LOG_ERROR("No memory for http ota!");
        ret = FAILURE_RET;
        goto __exit;
    }
    memset(buffer_read, 0x00, HTTP_OTA_BUFF_LEN);

    LOG_INFO("OTA file size is (%d)", file_size);
    do
    {
        length = IOT_OTA_FetchYield(h_ota, buffer_read, HTTP_OTA_BUFF_LEN, HTTP_OTA_RANGE_LEN, 10);
        if (length > 0)
        {
            /* Write the data to the corresponding partition address */
            if(HAL_Download_Write(download_handle, total_length, buffer_read, length) == FAILURE_RET){
                ret = FAILURE_RET;
                goto __exit;
            }

            total_length += length;
            //wait cancel cmd
            IOT_OTA_Yield(handle, 100);
        }
        else
        {
            LOG_ERROR("Exit: server return err (%d)!", length);
            ret = ERR_OTA_FETCH_FAILED;                
            goto __exit;
        }
    } while (!IOT_OTA_IsFetchFinish(h_ota));

    if (total_length == file_size)
    {    
        ret = SUCCESS_RET;
        IOT_OTA_Ioctl(h_ota, OTA_IOCTL_CHECK_FIRMWARE, &firmware_valid, 4);
        if (0 == firmware_valid) {
            LOG_ERROR("The firmware is invalid"); 
            ret = IOT_OTA_GetLastError(h_ota);
            goto __exit;
        } else {
            LOG_INFO("The firmware is valid");            
            IOT_OTA_Ioctl(h_ota, OTA_IOCTL_VERSION, msg_version, 33);
            IOT_OTA_ReportSuccess(h_ota, msg_version);
        }
        
        if(HAL_Download_End(download_handle))
            ret = FAILURE_RET;

        LOG_INFO("Download firmware to flash success.");
    }

__exit:
    if (buffer_read != NULL)
        HAL_Free(buffer_read);    
    IOT_OTA_Clear(h_ota);
    
    return ret;
}


