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

#ifndef C_SDK_UIOT_EXPORT_FILE_UPLOAD_
#define C_SDK_UIOT_EXPORT_FILE_UPLOAD_

#if defined(__cplusplus)
extern "C" {
#endif

int IOT_GET_URL_AND_AUTH(const char *product_sn, const char *device_sn, const char *device_sercret, char *file_path, char *md5, char *authorization, char *put_url);

int IOT_UPLOAD_FILE(char *file_path, char *md5, char *authorization, char *url, uint32_t timeout_ms);

#if defined(__cplusplus)
}
#endif

#endif 
