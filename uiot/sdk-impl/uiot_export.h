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

#ifndef C_SDK_UIOT_EXPORT_H_
#define C_SDK_UIOT_EXPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* MQTT心跳消息发送周期, 单位:s */
#define UIOT_MQTT_KEEP_ALIVE_INTERNAL                               (240)

/* MQTT 阻塞调用(包括连接, 订阅, 发布等)的超时时间 */
#define UIOT_MQTT_COMMAND_TIMEOUT                                   (5 * 1000)

/* 接收到 MQTT 包头以后，接收剩余长度及剩余包，最大延迟等待时延 */
#define UIOT_MQTT_MAX_REMAIN_WAIT_MS                                (2000)

/* MQTT消息发送buffer大小, 支持最大256*1024 */
#define UIOT_MQTT_TX_BUF_LEN                                        (2048)

/* MQTT消息接收buffer大小, 支持最大256*1024 */
#define UIOT_MQTT_RX_BUF_LEN                                        (2048)

/* 重连最大等待时间 */
#define MAX_RECONNECT_WAIT_INTERVAL                                 (60 * 1000)

/* 使能无限重连，0表示超过重连最大等待时间后放弃重连，
 * 1表示超过重连最大等待时间后以固定间隔尝试重连*/
#define ENABLE_INFINITE_RECONNECT                                    1

/* MQTT连接域名 */
#define UIOT_MQTT_DIRECT_DOMAIN                                      "mqtt-cn-sh2.iot.ucloud.cn" //"pre-mqtt.iot.ucloud.cn" 


#include "uiot_export_mqtt.h"
#include "uiot_defs.h"

#ifdef __cplusplus
}
#endif

#endif /* C_SDK_UIOT_EXPORT_H_ */
