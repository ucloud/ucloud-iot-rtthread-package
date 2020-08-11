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

#include <string.h>
#include <stdio.h>

#include "ota_config.h"
#include "ota_internal.h"

#include "utils_md5.h"
#include "lite-utils.h"


void *ota_lib_md5_init(void) {
    iot_md5_context *ctx = HAL_Malloc(sizeof(iot_md5_context));
    if (NULL == ctx) {
        return NULL;
    }

    utils_md5_init(ctx);
    utils_md5_starts(ctx);

    return ctx;
}


void ota_lib_md5_update(void *md5, const char *buf, size_t buf_len) {
    utils_md5_update(md5, (unsigned char *) buf, buf_len);
}


void ota_lib_md5_finalize(void *md5, char *output_str) {
    utils_md5_finish_hb2hex(md5, output_str);
}


void ota_lib_md5_deinit(void *md5) {
    if (NULL != md5) {
        HAL_Free(md5);
    }
}


int ota_lib_get_msg_type(char *json, char **type) {
    FUNC_ENTRY;

    if (NULL == (*type = LITE_json_value_of(TYPE_FIELD, json))) {
        LOG_ERROR("get value of type key failed");
        FUNC_EXIT_RC(ERR_OTA_GENERAL_FAILURE);
    }

    FUNC_EXIT_RC(SUCCESS_RET);
}

int ota_lib_get_msg_module_ver(char *json, char **module, char **ver) {
    FUNC_ENTRY;

    if (NULL == (*module = LITE_json_value_of(MODULE_FIELD, json))) {
        LOG_ERROR("get value of module key failed");
        FUNC_EXIT_RC(ERR_OTA_GENERAL_FAILURE);
    }

    if (NULL == (*ver = LITE_json_value_of(VERSION_FIELD, json))) {        
        LOG_ERROR("get value of ver key failed");
        FUNC_EXIT_RC(ERR_OTA_GENERAL_FAILURE);
    }
    
    FUNC_EXIT_RC(SUCCESS_RET);
}

int ota_lib_get_params(char *json, char **url, char **module, char **download_name, char **version, char **md5,
                       uint32_t *fileSize) {
    FUNC_ENTRY;

    char *module_str;
    char *file_size_str;
    char *version_str;
    char *url_str;
    char *md5_str;

    /* get module */
    if (NULL == (module_str = LITE_json_value_of(MODULE_FIELD, json))) {
        LOG_ERROR("get value of module key failed");
        FUNC_EXIT_RC(ERR_OTA_GENERAL_FAILURE);
    }
    if (NULL != *module) {
        HAL_Free(*module);
    }
    *module = module_str;

    /* get version */
    if (NULL == (version_str = LITE_json_value_of(VERSION_FIELD, json))) {
        LOG_ERROR("get value of version key failed");
        FUNC_EXIT_RC(ERR_OTA_GENERAL_FAILURE);
    }
    if (NULL != *version) {
        HAL_Free(*version);
    }
    *version = version_str;

    /* get URL */
    if (NULL == (url_str = LITE_json_value_of(URL_FIELD, json))) {
        LOG_ERROR("get value of url key failed");
        FUNC_EXIT_RC(ERR_OTA_GENERAL_FAILURE);
    }
    if (NULL != *url) {
        HAL_Free(*url);
    }
    *url = url_str;

    *download_name = HAL_Download_Name_Set((void*)url_str);
    
    /* get md5 */
    if (NULL == (md5_str = LITE_json_value_of(MD5_FIELD, json))) {
        LOG_ERROR("get value of md5 failed");
        FUNC_EXIT_RC(ERR_OTA_GENERAL_FAILURE);
    }
    if (NULL != *md5) {
        HAL_Free(*md5);
    }
    *md5 = md5_str;

    /* get file size */
    if (NULL == (file_size_str = LITE_json_value_of(SIZE_FIELD, json))) {
        LOG_ERROR("get value of file size failed");
        FUNC_EXIT_RC(ERR_OTA_GENERAL_FAILURE);
    }

    if (SUCCESS_RET != LITE_get_uint32(fileSize, file_size_str)) {
        LOG_ERROR("get uint32 failed");
        HAL_Free(file_size_str);
        FUNC_EXIT_RC(ERR_OTA_GENERAL_FAILURE);
    }
    HAL_Free(file_size_str);
    FUNC_EXIT_RC(SUCCESS_RET);
}

int ota_lib_gen_upstream_msg(char *buf, size_t bufLen, const char *module, const char *version, int progress,
                             IOT_OTA_UpstreamMsgType reportType) {
    FUNC_ENTRY;

    int ret;

    switch (reportType) {
        case OTA_REPORT_DOWNLOAD_TIMEOUT:
        case OTA_REPORT_FIRMWARE_NOT_EXIST:
        case OTA_REPORT_MD5_MISMATCH:
        case OTA_REPORT_SIGNATURE_EXPIRED:
        case OTA_REPORT_FIRMWARE_BURN_FAILED:
        case OTA_REPORT_UNDEFINED_ERROR:
            ret = HAL_Snprintf(buf, bufLen, REPORT_FAIL_MSG_TEMPLATE, reportType);
            break;

        case OTA_REPORT_DOWNLOADING:
            ret = HAL_Snprintf(buf, bufLen, REPORT_PROGRESS_MSG_TEMPLATE, "downloading", progress, module, version);
            break;

        case OTA_REPORT_BURNING:
            ret = HAL_Snprintf(buf, bufLen, REPORT_PROGRESS_MSG_TEMPLATE, "burning", progress, module, version);
            break;

        case OTA_REPORT_SUCCESS:
            ret = HAL_Snprintf(buf, bufLen, REPORT_SUCCESS_MSG_TEMPLATE, module, version);
            break;

        case OTA_REQUEST_FIRMWARE:
            ret = HAL_Snprintf(buf, bufLen, REQUEST_FIRMWARE_MSG_TEMPLATE, module, version);
            break;

        case OTA_REPORT_VERSION:
            ret = HAL_Snprintf(buf, bufLen, REPORT_VERSION_MSG_TEMPLATE, module, version);
            break;

        default: FUNC_EXIT_RC(ERR_OTA_GENERAL_FAILURE);
    }

    if (ret < 0) {
        LOG_ERROR("HAL_Snprintf failed");
        FUNC_EXIT_RC(ERR_OTA_GENERAL_FAILURE);
    } else if (ret >= bufLen) {
        LOG_ERROR("msg is too long");
        FUNC_EXIT_RC(ERR_OTA_STR_TOO_LONG);
    }

    FUNC_EXIT_RC(SUCCESS_RET);
}
