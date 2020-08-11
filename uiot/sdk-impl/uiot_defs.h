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

#ifndef C_SDK_UIOT_DEFS_H_
#define C_SDK_UIOT_DEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define IOT_PRODUCT_SN_LEN             (16)
#define IOT_PRODUCT_SECRET_LEN         (16)

#define IOT_DEVICE_SN_LEN              (64)
#define IOT_DEVICE_SECRET_LEN          (16)


#ifndef _IN_
#define _IN_
#endif

#ifndef _OU_
#define _OU_
#endif

typedef enum {
    MQTT_ALREADY_CONNECTED                            = 4,       // 表示与MQTT服务器已经建立连接
    MQTT_CONNECTION_ACCEPTED                          = 3,       // 表示服务器接受客户端MQTT连接
    MQTT_MANUALLY_DISCONNECTED                        = 2,       // 表示与MQTT服务器已经手动断开
    MQTT_RECONNECTED                                  = 1,       // 表示与MQTT服务器重连成功

    SUCCESS_RET                                       = 0,       // 表示成功返回
    FAILURE_RET                                       = -1,      // 表示失败返回

    ERR_PARAM_INVALID                                 = -2,      // 表示参数无效错误

    ERR_HTTP_CLOSED                                   = -3,      // 远程主机关闭连接
    ERR_HTTP_UNKNOWN_ERROR                            = -4,      // HTTP未知错误
    ERR_HTTP_PROTOCOL_ERROR                           = -5,      // 协议错误
    ERR_HTTP_UNRESOLVED_DNS                           = -6,      // 域名解析失败
    ERR_HTTP_URL_PARSE_ERROR                          = -7,      // URL解析失败
    ERR_HTTP_CONN_ERROR                               = -8,      // HTTP连接失败
    ERR_HTTP_AUTH_ERROR                               = -9,      // HTTP鉴权问题
    ERR_HTTP_NOT_FOUND                                = -10,     // HTTP 404
    ERR_HTTP_TIMEOUT                                  = -11,     // HTTP 超时

    ERR_MQTT_PUSH_TO_LIST_FAILED                      = -102,    // 表示往等待 ACK 列表中添加元素失败
    ERR_MQTT_NO_CONN                                  = -103,    // 表示未与MQTT服务器建立连接或已经断开连接
    ERR_MQTT_UNKNOWN_ERROR                            = -104,    // 表示MQTT相关的未知错误
    ERR_MQTT_ATTEMPTING_RECONNECT                     = -105,    // 表示正在与MQTT服务重新建立连接
    ERR_MQTT_RECONNECT_TIMEOUT                        = -106,    // 表示重连已经超时
    ERR_MQTT_MAX_SUBSCRIPTIONS_REACHED                = -107,    // 表示超过可订阅的主题数
    ERR_MQTT_SUB_FAILED                               = -108,    // 表示订阅主题失败, 即服务器拒绝
    ERR_MQTT_NOTHING_TO_READ                          = -109,    // 表示无MQTT相关报文可以读取
    ERR_MQTT_PACKET_READ_ERROR                        = -110,    // 表示读取的MQTT报文有问题
    ERR_MQTT_REQUEST_TIMEOUT                          = -111,    // 表示MQTT相关操作请求超时
    ERR_MQTT_CONNACK_UNKNOWN                          = -112,    // 表示客户端MQTT连接未知错误
    ERR_MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION    = -113,    // 表示客户端MQTT版本错误
    ERR_MQTT_CONNACK_IDENTIFIER_REJECTED              = -114,    // 表示客户端标识符错误
    ERR_MQTT_CONNACK_SERVER_UNAVAILABLE               = -115,    // 表示服务器不可用
    ERR_MQTT_CONNACK_BAD_USERDATA                     = -116,    // 表示客户端连接参数中的username或password错误
    ERR_MQTT_CONNACK_NOT_AUTHORIZED                   = -117,    // 表示客户端连接认证失败
    ERR_MQTT_RX_MESSAGE_PACKET_TYPE_INVALID           = -118,    // 表示收到的消息无效
    ERR_MQTT_BUFFER_TOO_SHORT                         = -119,    // 表示消息接收缓冲区的长度小于消息的长度
    ERR_MQTT_QOS_NOT_SUPPORT                          = -120,    // 表示该QOS级别不支持
    ERR_MQTT_UNSUB_FAILED                             = -121,    // 表示取消订阅主题失败,比如该主题不存在

    ERR_JSON_PARSE                                    = -132,    // 表示JSON解析错误
    ERR_JSON_BUFFER_TRUNCATED                         = -133,    // 表示JSON文档会被截断
    ERR_JSON_BUFFER_TOO_SMALL                         = -134,    // 表示存储JSON文档的缓冲区太小
    ERR_JSON                                          = -135,    // 表示JSON文档生成错误
    ERR_MAX_JSON_TOKEN                                = -136,    // 表示超过JSON文档中的最大Token数
    ERR_MAX_APPENDING_REQUEST                         = -137,    // 表示超过同时最大的文档请求
    ERR_MAX_TOPIC_LENGTH                              = -138,    // 表示超过规定最大的topic长度

    ERR_SHADOW_PROPERTY_EXIST                         = -201,    // 表示注册的属性已经存在
    ERR_SHADOW_NOT_PROPERTY_EXIST                     = -202,    // 表示注册的属性不存在
    ERR_SHADOW_UPDATE_TIMEOUT                         = -203,    // 表示更新设备影子文档超时
    ERR_SHADOW_UPDATE_REJECTED                        = -204,    // 表示更新设备影子文档被拒绝
    ERR_SHADOW_GET_TIMEOUT                            = -205,    // 表示拉取设备影子文档超时
    ERR_SHADOW_GET_REJECTED                           = -206,    // 表示拉取设备影子文档被拒绝

    ERR_OTA_GENERAL_FAILURE                           = -301,    // 表示OTA通用FAILURE返回值
    ERR_OTA_INVALID_PARAM                             = -302,    // 表示OTA参数错误
    ERR_OTA_INVALID_STATE                             = -303,    // 表示OTA状态错误
    ERR_OTA_STR_TOO_LONG                              = -304,    // 表示OTA字符串长度超过限制
    ERR_OTA_FETCH_FAILED                              = -305,    // 表示固件下载失败
    ERR_OTA_FILE_NOT_EXIST                            = -306,    // 表示固件不存在
    ERR_OTA_FETCH_AUTH_FAIL                           = -307,    // 表示预签名认证失败
    ERR_OTA_FETCH_TIMEOUT                             = -308,    // 表示下载超时
    ERR_OTA_NO_MEMORY                                 = -309,    // 表示内存不足
    ERR_OTA_OSC_FAILED                                = -310,    // 表示OTA信号通道错误
    ERR_OTA_MD5_MISMATCH                              = -311,    // 表示MD5不匹配


    ERR_TCP_SOCKET_FAILED                             = -601,    // 表示TCP连接建立套接字失败
    ERR_TCP_UNKNOWN_HOST                              = -602,    // 表示无法通过主机名获取IP地址
    ERR_TCP_CONNECT_FAILED                            = -603,    // 表示建立TCP连接失败
    ERR_TCP_READ_TIMEOUT                              = -604,    // 表示TCP读超时
    ERR_TCP_WRITE_TIMEOUT                             = -605,    // 表示TCP写超时
    ERR_TCP_READ_FAILED                               = -606,    // 表示TCP读错误
    ERR_TCP_WRITE_FAILED                              = -607,    // 表示TCP写错误
    ERR_TCP_PEER_SHUTDOWN                             = -608,    // 表示TCP对端关闭了连接
    ERR_TCP_NOTHING_TO_READ                           = -609,    // 表示底层没有数据可以读取

    ERR_SSL_INIT_FAILED                               = -701,    // 表示SSL初始化失败
    ERR_SSL_CERT_FAILED                               = -702,    // 表示SSL证书相关问题
    ERR_SSL_CONNECT_FAILED                            = -703,    // 表示SSL连接失败
    ERR_SSL_CONNECT_TIMEOUT                           = -704,    // 表示SSL连接超时
    ERR_SSL_WRITE_TIMEOUT                             = -705,    // 表示SSL写超时
    ERR_SSL_WRITE_FAILED                              = -706,    // 表示SSL写错误
    ERR_SSL_READ_TIMEOUT                              = -707,    // 表示SSL读超时
    ERR_SSL_READ_FAILED                               = -708,    // 表示SSL读错误
    ERR_SSL_NOTHING_TO_READ                           = -709,    // 表示底层没有数据可以读取
} IoT_Error_t;

#define Max(a,b) ((a) > (b) ? (a) : (b))
#define Min(a,b) ((a) < (b) ? (a) : (b))

#ifdef ENABLE_LOG_DEBUG
#define LOG_DEBUG(...)    \
    {\
    HAL_Printf("DEBUG:   %s L#%d ", __func__, __LINE__);  \
    HAL_Printf(__VA_ARGS__); \
    HAL_Printf("\r\n"); \
    }
#else
#define LOG_DEBUG(...)
#endif

/**
 * @brief Info level logging macro.
 *
 * Macro to expose desired log message.  Info messages do not include automatic function names and line numbers.
 */
#ifdef ENABLE_LOG_INFO
#define LOG_INFO(...)    \
    {\
    HAL_Printf(__VA_ARGS__); \
    HAL_Printf("\r\n"); \
    }
#else
#define LOG_INFO(...)
#endif

/**
 * @brief Warn level logging macro.
 *
 * Macro to expose function, line number as well as desired log message.
 */
#ifdef ENABLE_LOG_WARN
#define LOG_WARN(...)   \
    { \
    HAL_Printf("WARN:  %s L#%d ", __func__, __LINE__);  \
    HAL_Printf(__VA_ARGS__); \
    HAL_Printf("\r\n"); \
    }
#else
#define LOG_WARN(...)
#endif

/**
 * @brief Error level logging macro.
 *
 * Macro to expose function, line number as well as desired log message.
 */
#ifdef ENABLE_LOG_ERROR
#define LOG_ERROR(...)  \
    { \
    HAL_Printf("ERROR: %s L#%d ", __func__, __LINE__); \
    HAL_Printf(__VA_ARGS__); \
    HAL_Printf("\r\n"); \
    }
#else
#define LOG_ERROR(...)
#endif

#ifdef ENABLE_IOT_TRACE
#define FUNC_ENTRY    \
    {\
    HAL_Printf("FUNC_ENTRY:   %s L#%d \r\n", __func__, __LINE__);  \
    }
#define FUNC_EXIT    \
    {\
    HAL_Printf("FUNC_EXIT:   %s L#%d \r\n", __func__, __LINE__);  \
    return; \
    }
#define FUNC_EXIT_RC(x)    \
    {\
    HAL_Printf("FUNC_EXIT:   %s L#%d Return Code : %d \r\n", __func__, __LINE__, x);  \
    return x; \
    }
#else
#define FUNC_ENTRY

#define FUNC_EXIT       { return; }
#define FUNC_EXIT_RC(x) { return x; }
#endif

#ifdef __cplusplus
}
#endif

#endif //C_SDK_UIOT_DEFS_H_
