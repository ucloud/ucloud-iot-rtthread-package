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

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <limits.h>

#include "mqtt_client.h"


#define MQTT_CONNECT_FLAG_USERNAME      0x80
#define MQTT_CONNECT_FLAG_PASSWORD      0x40
#define MQTT_CONNECT_FLAG_WILL_RETAIN   0x20
#define MQTT_CONNECT_FLAG_WILL_QOS2     0x18
#define MQTT_CONNECT_FLAG_WILL_QOS1     0x08
#define MQTT_CONNECT_FLAG_WILL_QOS0     0x00
#define MQTT_CONNECT_FLAG_WILL_FLAG     0x04
#define MQTT_CONNECT_FLAG_CLEAN_SES     0x02

#define MQTT_CONNACK_FLAG_SES_PRE       0x01

/**
 * Connect return code
 */
typedef enum {
    CONNACK_CONNECTION_ACCEPTED = 0,    // connection accepted
    CONANCK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR = 1,    // connection refused: unaccpeted protocol verison
    CONNACK_IDENTIFIER_REJECTED_ERROR = 2,    // connection refused: identifier rejected
    CONNACK_SERVER_UNAVAILABLE_ERROR = 3,    // connection refused: server unavailable
    CONNACK_BAD_USERDATA_ERROR = 4,    // connection refused: bad user name or password
    CONNACK_NOT_AUTHORIZED_ERROR = 5     // connection refused: not authorized
} MQTTConnackReturnCodes;

/**
  * Determines the length of the MQTT connect packet that would be produced using the supplied connect options.
  * @param options the options to be used to build the connect packet
  * @param the length of buffer needed to contain the serialized version of the packet
  * @return int indicating function execution status
  */
static uint32_t _get_packet_connect_rem_len(MQTTConnectParams *options) {
    size_t len = 0;
    /* variable depending on MQTT or MQIsdp */
    if (3 == options->mqtt_version) {
        len = 12;
    } else if (4 == options->mqtt_version) {
        len = 10;
    }

    len += strlen(options->client_id) + 2;

    if (options->username) {
        len += strlen(options->username) + 2;
    }

    if (options->password) {
        len += strlen(options->password) + 2;
    }

    return (uint32_t) len;
}

static void _copy_connect_params(MQTTConnectParams *destination, MQTTConnectParams *source) {
    POINTER_VALID_CHECK_RTN(destination);
    POINTER_VALID_CHECK_RTN(source);

    destination->mqtt_version = source->mqtt_version;
    destination->client_id = source->client_id;
    destination->username = source->username;
    destination->keep_alive_interval = source->keep_alive_interval;
    destination->clean_session = source->clean_session;
    destination->auto_connect_enable = source->auto_connect_enable;
    destination->password = source->password;
}

/**
  * Serializes the connect options into the buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param len the length in bytes of the supplied buffer
  * @param options the options to be used to build the connect packet
  * @param serialized length
  * @return int indicating function execution status
  */
static int _serialize_connect_packet(unsigned char *buf, size_t buf_len, MQTTConnectParams *options, uint32_t *serialized_len) {
    POINTER_VALID_CHECK(buf, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(options, ERR_PARAM_INVALID);
    STRING_PTR_VALID_CHECK(options->client_id, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(serialized_len, ERR_PARAM_INVALID);

    unsigned char *ptr = buf;
    unsigned char header = 0;
    unsigned char flags = 0;
    uint32_t rem_len = 0;
    int ret;

    rem_len = _get_packet_connect_rem_len(options);
    if (get_mqtt_packet_len(rem_len) > buf_len) {
        return ERR_MQTT_BUFFER_TOO_SHORT;
    }

    ret = mqtt_init_packet_header(&header, CONNECT, QOS0, 0, 0);
    if (SUCCESS_RET != ret) {
        return ret;
    }

    // 报文固定头部第一个字节
    mqtt_write_char(&ptr, header);

    // 报文固定头部剩余长度字段
    ptr += mqtt_write_packet_rem_len(ptr, rem_len);

    // 报文可变头部协议名 + 协议版本号
    if (4 == options->mqtt_version) {
        mqtt_write_utf8_string(&ptr, "MQTT");
        mqtt_write_char(&ptr, (unsigned char) 4);
    } else {
        mqtt_write_utf8_string(&ptr, "MQIsdp");
        mqtt_write_char(&ptr, (unsigned char) 3);
    }

    // 报文可变头部连接标识位
    
    flags |= (options->clean_session) ? MQTT_CONNECT_FLAG_CLEAN_SES : 0;
    flags |= (options->username != NULL) ? MQTT_CONNECT_FLAG_USERNAME : 0;

    flags |= MQTT_CONNECT_FLAG_PASSWORD;
    
    mqtt_write_char(&ptr, flags);

    // 报文可变头部心跳周期/保持连接, 一个以秒为单位的时间间隔, 表示为一个16位的字
    mqtt_write_uint_16(&ptr, options->keep_alive_interval);

    // 有效负载部分: 客户端标识符
    mqtt_write_utf8_string(&ptr, options->client_id);

    // 用户名
    if ((flags & MQTT_CONNECT_FLAG_USERNAME) && options->username != NULL) {
        mqtt_write_utf8_string(&ptr, options->username);
    }

    if ((flags & MQTT_CONNECT_FLAG_PASSWORD) && options->password != NULL) {
        mqtt_write_utf8_string(&ptr, options->password);
    }

    *serialized_len = (uint32_t) (ptr - buf);
    
    return SUCCESS_RET;
}

/**
  * Deserializes the supplied (wire) buffer into connack data - return code
  * @param sessionPresent the session present flag returned (only for MQTT 3.1.1)
  * @param connack_rc returned integer value of the connack return code
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return int indicating function execution status
  */
static int _deserialize_connack_packet(uint8_t *sessionPresent, int *connack_rc, unsigned char *buf, size_t buflen) {
    POINTER_VALID_CHECK(sessionPresent, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(connack_rc, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(buf, ERR_PARAM_INVALID);

    unsigned char header, type = 0;
    unsigned char *curdata = buf;
    unsigned char *enddata = NULL;
    int ret;
    uint32_t decodedLen = 0, readBytesLen = 0;
    unsigned char flags = 0;
    unsigned char connack_rc_char;

    // CONNACK 头部大小是固定的2字节长度, 可变头部也是两个字节的长度, 无有效负载
    if (4 > buflen) {
        return ERR_MQTT_BUFFER_TOO_SHORT;
    }

    // 读取固定头部第一个字节
    header = mqtt_read_char(&curdata);
    type = (header&MQTT_HEADER_TYPE_MASK)>>MQTT_HEADER_TYPE_SHIFT;
    if (CONNACK != type) {
        return FAILURE_RET;
    }

    // 读取固定头部剩余长度字段
    ret = mqtt_read_packet_rem_len_form_buf(curdata, &decodedLen, &readBytesLen);
    if (SUCCESS_RET != ret) {
        return ret;
    }
    curdata += (readBytesLen);
    enddata = curdata + decodedLen;
    if (enddata - curdata != 2) {
        return FAILURE_RET;
    }

    // 读取可变头部-连接确认标志 参考MQTT协议说明文档3.2.2.1小结
    flags = mqtt_read_char(&curdata);
    *sessionPresent = flags & MQTT_CONNACK_FLAG_SES_PRE;

    // 读取可变头部-连接返回码 参考MQTT协议说明文档3.2.2.3小结
    connack_rc_char = mqtt_read_char(&curdata);
    switch (connack_rc_char) {
        case CONNACK_CONNECTION_ACCEPTED:
            *connack_rc = MQTT_CONNECTION_ACCEPTED;
            break;
        case CONANCK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR:
            *connack_rc = ERR_MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION;
            break;
        case CONNACK_IDENTIFIER_REJECTED_ERROR:
            *connack_rc = ERR_MQTT_CONNACK_IDENTIFIER_REJECTED;
            break;
        case CONNACK_SERVER_UNAVAILABLE_ERROR:
            *connack_rc = ERR_MQTT_CONNACK_SERVER_UNAVAILABLE;
            break;
        case CONNACK_BAD_USERDATA_ERROR:
            *connack_rc = ERR_MQTT_CONNACK_BAD_USERDATA;
            break;
        case CONNACK_NOT_AUTHORIZED_ERROR:
            *connack_rc = ERR_MQTT_CONNACK_NOT_AUTHORIZED;
            break;
        default:
            *connack_rc = ERR_MQTT_CONNACK_UNKNOWN;
            break;
    }

    return SUCCESS_RET;
}

/**
 * @brief 与服务器建立MQTT连接
 *
 * @param pClient
 * @param options
 * @return
 */
static int _mqtt_connect(UIoT_Client *pClient, MQTTConnectParams *options) {
    Timer connect_timer;
    int connack_rc = FAILURE_RET, ret = FAILURE_RET;
    uint8_t sessionPresent = 0;
    uint32_t len = 0;

    init_timer(&connect_timer);
    countdown_ms(&connect_timer, pClient->command_timeout_ms);

    if (NULL != options) {
        _copy_connect_params(&(pClient->options), options);
    }

    // 根据连接参数，建立TLS或NOTLS连接
    ret = pClient->network_stack.connect(&(pClient->network_stack));
    if (SUCCESS_RET != ret) {
        return ret;
    }

    HAL_MutexLock(pClient->lock_write_buf);
    // 序列化CONNECT报文
    ret = _serialize_connect_packet(pClient->write_buf, pClient->write_buf_size, &(pClient->options), &len);
    if (SUCCESS_RET != ret || 0 == len) {
        HAL_MutexUnlock(pClient->lock_write_buf);
        return ret;
    }

    // 发送CONNECT报文
    ret = send_mqtt_packet(pClient, len, &connect_timer);
    if (SUCCESS_RET != ret) {
        HAL_MutexUnlock(pClient->lock_write_buf);
        return ret;
    }
    HAL_MutexUnlock(pClient->lock_write_buf);

    // 阻塞等待CONNACK的报文,
    ret = wait_for_read(pClient, CONNACK, &connect_timer, 0);
    if (SUCCESS_RET != ret) {
        return ret;
    }

    // 反序列化CONNACK包, 检查返回码
    ret = _deserialize_connack_packet(&sessionPresent, &connack_rc, pClient->read_buf, pClient->read_buf_size);
    if (SUCCESS_RET != ret) {
        return ret;
    }

    if (MQTT_CONNECTION_ACCEPTED != connack_rc) {
        return connack_rc;
    }

    set_client_conn_state(pClient, CONNECTED);
    HAL_MutexLock(pClient->lock_generic);
    pClient->was_manually_disconnected = 0;
    pClient->is_ping_outstanding = 0;
    countdown(&pClient->ping_timer, pClient->options.keep_alive_interval);
    HAL_MutexUnlock(pClient->lock_generic);

    return SUCCESS_RET;
}

int uiot_mqtt_connect(UIoT_Client *pClient, MQTTConnectParams *pParams) {
    int ret;
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pParams, ERR_PARAM_INVALID);

    // 如果MQTT连接已经建立, 不要重复发送CONNECT报文
    if (get_client_conn_state(pClient)) {
        return MQTT_ALREADY_CONNECTED;
    }

    ret = _mqtt_connect(pClient, pParams);

    // 如果MQTT连接建立失败, 则断开底层的TLS连接
    if (ret != SUCCESS_RET) {
        pClient->network_stack.disconnect(&(pClient->network_stack));
    }

    return ret;
}

int uiot_mqtt_attempt_reconnect(UIoT_Client *pClient) {
    int ret;
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);

    LOG_INFO("attempt to reconnect...");

    if (get_client_conn_state(pClient)) {
        return MQTT_ALREADY_CONNECTED;
    }

    ret = uiot_mqtt_connect(pClient, &pClient->options);

    if (!get_client_conn_state(pClient)) {
        return ret;
    }

    ret = uiot_mqtt_resubscribe(pClient);
    if (ret != SUCCESS_RET) {
        return ret;
    }

    return MQTT_RECONNECTED;
}

int uiot_mqtt_disconnect(UIoT_Client *pClient) {
    int ret;
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);

    Timer timer;
    uint32_t serialized_len = 0;

    if (get_client_conn_state(pClient) == 0) {
        return ERR_MQTT_NO_CONN;
    }

    // 1. 组disconnect包
    HAL_MutexLock(pClient->lock_write_buf);
    ret = serialize_packet_with_zero_payload(pClient->write_buf, pClient->write_buf_size, DISCONNECT, &serialized_len);
    if (ret != SUCCESS_RET) {
        HAL_MutexUnlock(pClient->lock_write_buf);
        return ret;
    }

    init_timer(&timer);
    countdown_ms(&timer, pClient->command_timeout_ms);

    // 2. 发送disconnect包
    if (serialized_len > 0) {
        ret = send_mqtt_packet(pClient, serialized_len, &timer);
        if (ret != SUCCESS_RET) {
            HAL_MutexUnlock(pClient->lock_write_buf);
            return ret;
        }
    }
    HAL_MutexUnlock(pClient->lock_write_buf);

    // 3. 断开底层TCP连接, 并修改相关标识位
    pClient->network_stack.disconnect(&(pClient->network_stack));
    set_client_conn_state(pClient, DISCONNECTED);
    pClient->was_manually_disconnected = 1;

    LOG_INFO("mqtt disconnect!");

    return SUCCESS_RET;
}

#ifdef __cplusplus
}
#endif


