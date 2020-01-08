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

#ifndef C_SDK_OTA_CONFIG_H_
#define C_SDK_OTA_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define TYPE_FIELD 				    "Method"
#define MD5_FIELD				    "Payload.MD5"
#define VERSION_FIELD			    "Payload.Version"
#define URL_FIELD				    "Payload.URL"
#define SIZE_FIELD			        "Payload.Size"
#define UPDATE_FIRMWARE_METHOD      "update_firmware"

#define REPORT_PROGRESS_MSG_TEMPLATE      "{\"Method\": \"report_progress\", \"Payload\": {\"State\":\"%s\", \"Percent\":%d}}"
#define REPORT_SUCCESS_MSG_TEMPLATE       "{\"Method\": \"report_success\", \"Payload\":{\"Version\":\"%s\"}}"
#define REPORT_FAIL_MSG_TEMPLATE          "{\"Method\": \"report_fail\", \"Payload\": {\"ErrCode\": %d}}"
#define REPORT_VERSION_MSG_TEMPLATE       "{\"Method\": \"report_version\", \"Payload\":{\"Version\":\"%s\"}}"
#define REQUEST_FIRMWARE_MSG_TEMPLATE     "{\"Method\": \"request_firmware\", \"Payload\":{\"Version\":\"%s\"}}"

#define OTA_VERSION_STR_LEN_MIN     (1)
#define OTA_VERSION_STR_LEN_MAX     (32)
#define OTA_UPSTREAM_MSG_BUF_LEN    (129)
#define OTA_TOPIC_BUF_LEN           (129)

#define OTA_UPSTREAM_TOPIC_TYPE      "upstream"
#define OTA_DOWNSTREAM_TOPIC_TYPE    "downstream"
#define OTA_TOPIC_TEMPLATE           "/$system/%s/%s/ota/%s"

#define OTA_REPORT_PROGRESS_INTERVAL 5  //下载固件过程中，上报progress的时间间隔，单位: s

#ifdef __cplusplus
}
#endif

#endif //C_SDK_OTA_CONFIG_H_
