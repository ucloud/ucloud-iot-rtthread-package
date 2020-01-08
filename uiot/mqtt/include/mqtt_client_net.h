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


#ifndef C_SDK_MQTT_CLIENT_NET_H_
#define C_SDK_MQTT_CLIENT_NET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "utils_net.h"

/**
 * @brief 初始化TLS实现
 *
 * @param pNetwork 网络操作相关结构体
 * @return 返回0, 表示初始化成功
 */
int uiot_mqtt_network_init(utils_network_pt pNetwork, const char *host, uint16_t port, uint16_t authmode, const char *ca_crt);

#ifdef __cplusplus
}
#endif

#endif //C_SDK_MQTT_CLIENT_NET_H_
