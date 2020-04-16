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
#include <time.h>

#include "mqtt_client.h"

#include "utils_list.h"
#include "lite-utils.h"


#define MAX_NO_OF_REMAINING_LENGTH_BYTES 4

/* return: 0, identical; NOT 0, different. */
static int _check_handle_is_identical(SubTopicHandle *sub_handle1, SubTopicHandle *sub_handle2)
{
    if (!sub_handle1 || !sub_handle2) {
        return 1;
    }

    int topic_name_Len = strlen(sub_handle1->topic_filter);

    if (topic_name_Len != strlen(sub_handle2->topic_filter)) {
        return 1;
    }

    if (0 != strncmp(sub_handle1->topic_filter, sub_handle2->topic_filter, topic_name_Len)) {
        return 1;
    }

    if (sub_handle1->message_handler != sub_handle2->message_handler) {
        return 1;
    }

    return 0;
}

uint16_t get_next_packet_id(UIoT_Client *pClient) {
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);

    HAL_MutexLock(pClient->lock_generic);
    pClient->next_packet_id = (uint16_t) ((MAX_PACKET_ID == pClient->next_packet_id) ? 1 : (pClient->next_packet_id + 1));
    HAL_MutexUnlock(pClient->lock_generic);

    return pClient->next_packet_id;
}

/**
 * Encodes the message length according to the MQTT algorithm
 * @param buf the buffer into which the encoded data is written
 * @param length the length to be encoded
 * @return the number of bytes written to buffer
 */
size_t mqtt_write_packet_rem_len(unsigned char *buf, uint32_t length) {
    size_t outLen = 0;

    do {
        unsigned char encodeByte;
        encodeByte = (unsigned char) (length % 128);
        length /= 128;
        /* if there are more digits to encode, set the top bit of this digit */
        if (length > 0) {
            encodeByte |= 0x80;
        }
        buf[outLen++] = encodeByte;
    } while (length > 0);

    return (int)outLen;
}

size_t get_mqtt_packet_len(size_t rem_len) {
    rem_len += 1; /* header byte */

    /* now remaining_length field */
    if (rem_len < 128) {
        rem_len += 1;
    } else if (rem_len < 16384) {
        rem_len += 2;
    } else if (rem_len < 2097151) {
        rem_len += 3;
    } else {
        rem_len += 4;
    }

    return rem_len;
}

/**
 * Decodes the message length according to the MQTT algorithm
 * @param getcharfn pointer to function to read the next character from the data source
 * @param value the decoded length returned
 * @return the number of bytes read from the socket
 */
static int _decode_packet_rem_len_from_buf_read(uint32_t (*getcharfn)(unsigned char *, uint32_t), uint32_t *value,
                                                uint32_t *readBytesLen) {
    unsigned char c;
    uint32_t multiplier = 1;
    uint32_t len = 0;
    *value = 0;
    do {
        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES) {
            /* bad data */
            return ERR_MQTT_PACKET_READ_ERROR;
        }
        uint32_t getLen = 0;
        getLen = (*getcharfn)(&c, 1);
        if (1 != getLen) {
            return FAILURE_RET;
        }
        *value += (c & 127) * multiplier;
        multiplier *= 128;
    } while ((c & 128) != 0);

    *readBytesLen = len;

    return SUCCESS_RET;
}

static unsigned char *bufptr;
uint32_t bufchar(unsigned char *c, uint32_t count) {
    uint32_t i;

    for (i = 0; i < count; ++i) {
        *c = *bufptr++;
    }

    return count;
}

int mqtt_read_packet_rem_len_form_buf(unsigned char *buf, uint32_t *value, uint32_t *readBytesLen) {
    bufptr = buf;
    return _decode_packet_rem_len_from_buf_read(bufchar, value, readBytesLen);
}

/**
 * Calculates uint16 packet id from two bytes read from the input buffer
 * @param pptr pointer to the input buffer - incremented by the number of bytes used & returned
 * @return the value calculated
 */
uint16_t mqtt_read_uint16_t(unsigned char **pptr) {
    unsigned char *ptr = *pptr;
    uint8_t firstByte = (uint8_t) (*ptr);
    uint8_t secondByte = (uint8_t) (*(ptr + 1));
    uint16_t len = (uint16_t) (secondByte + (256 * firstByte));
    *pptr += 2;
    return len;
}

/**
 * Reads one character from the input buffer.
 * @param pptr pointer to the input buffer - incremented by the number of bytes used & returned
 * @return the character read
 */
unsigned char mqtt_read_char(unsigned char **pptr) {
    unsigned char c = **pptr;
    (*pptr)++;
    return c;
}

/**
 * Writes one character to an output buffer.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param c the character to write
 */
void mqtt_write_char(unsigned char **pptr, unsigned char c) {
    **pptr = c;
    (*pptr)++;
}

/**
 * Writes an integer as 2 bytes to an output buffer.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param anInt the integer to write
 */
void mqtt_write_uint_16(unsigned char **pptr, uint16_t anInt) {
    **pptr = (unsigned char) (anInt / 256);
    (*pptr)++;
    **pptr = (unsigned char) (anInt % 256);
    (*pptr)++;
}

/**
 * Writes a "UTF" string to an output buffer.  Converts C string to length-delimited.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param string the C string to write
 */
void mqtt_write_utf8_string(unsigned char **pptr, const char *string) {
    size_t len = strlen(string);
    mqtt_write_uint_16(pptr, (uint16_t) len);
    memcpy(*pptr, string, len);
    *pptr += len;
}

/**
 * Initialize the MQTT Header fixed byte. Used to ensure that Header bits are
 */
int mqtt_init_packet_header(unsigned char *header, MessageTypes message_type,
                            QoS Qos, uint8_t dup, uint8_t retained)
{
    POINTER_VALID_CHECK(header, ERR_PARAM_INVALID);
    unsigned char type, qos;

    switch (message_type) {
        case RESERVED:
            /* Should never happen */
            return ERR_MQTT_UNKNOWN_ERROR;
        case CONNECT:
            type = 0x01;
            break;
        case CONNACK:
            type = 0x02;
            break;
        case PUBLISH:
            type = 0x03;
            break;
        case PUBACK:
            type = 0x04;
            break;
        case PUBREC:
            type = 0x05;
            break;
        case PUBREL:
            type = 0x06;
            break;
        case PUBCOMP:
            type = 0x07;
            break;
        case SUBSCRIBE:
            type = 0x08;
            break;
        case SUBACK:
            type = 0x09;
            break;
        case UNSUBSCRIBE:
            type = 0x0A;
            break;
        case UNSUBACK:
            type = 0x0B;
            break;
        case PINGREQ:
            type = 0x0C;
            break;
        case PINGRESP:
            type = 0x0D;
            break;
        case DISCONNECT:
            type = 0x0E;
            break;
        default:
            /* Should never happen */
            return ERR_MQTT_UNKNOWN_ERROR;
    }

    switch (Qos) {
        case QOS0:
            qos = 0x00;
            break;
        case QOS1:
            qos = 0x01;
            break;
        case QOS2:
            qos = 0x02;
            break;
        default:
            /* Using QOS0 as default */
            qos = 0x00;
            break;
    }

    /* Generate the final protocol header by using bitwise operator */
    *header  = ((type<<MQTT_HEADER_TYPE_SHIFT)&MQTT_HEADER_TYPE_MASK)
                | ((dup<<MQTT_HEADER_DUP_SHIFT)&MQTT_HEADER_DUP_MASK)
                | ((qos<<MQTT_HEADER_QOS_SHIFT)&MQTT_HEADER_QOS_MASK)
                | (retained&MQTT_HEADER_RETAIN_MASK);

    return SUCCESS_RET;
}

/**
  * Deserializes the supplied (wire) buffer into an ack
  * @param packet_type returned integer - the MQTT packet type
  * @param dup returned integer - the MQTT dup flag
  * @param packet_id returned integer - the MQTT packet identifier
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buf_len the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int deserialize_ack_packet(uint8_t *packet_type, uint8_t *dup, uint16_t *packet_id, unsigned char *buf, size_t buf_len) {
    POINTER_VALID_CHECK(packet_type, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(dup, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(packet_id, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(buf, ERR_PARAM_INVALID);

    int ret;
    unsigned char header = 0;
    unsigned char *curdata = buf;
    unsigned char *enddata = NULL;
    uint32_t decodedLen = 0, readBytesLen = 0;

    /* PUBACK fixed header size is two bytes, variable header is 2 bytes, MQTT v3.1.1 Specification 3.4.1 */
    if (4 > buf_len) {
        return ERR_MQTT_BUFFER_TOO_SHORT;
    }

    header = mqtt_read_char(&curdata);        
    *packet_type = ((header&MQTT_HEADER_TYPE_MASK)>>MQTT_HEADER_TYPE_SHIFT);
    *dup  = ((header&MQTT_HEADER_DUP_MASK)>>MQTT_HEADER_DUP_SHIFT);

    /* read remaining length */
    ret = mqtt_read_packet_rem_len_form_buf(curdata, &decodedLen, &readBytesLen);
    if (SUCCESS_RET != ret) {
        return ret;
    }
    curdata += (readBytesLen);
    enddata = curdata + decodedLen;

    if (enddata - curdata < 2) {
        return FAILURE_RET;
    }

    *packet_id = mqtt_read_uint16_t(&curdata);
    
    // 返回错误码处理
    if (enddata - curdata >= 1) {
        unsigned char ack_code = mqtt_read_char(&curdata);
        if (ack_code != 0) {
            LOG_ERROR("deserialize_ack_packet failure! ack_code = 0x%02x", ack_code);
            return FAILURE_RET;
        }
    }

    return SUCCESS_RET;
}

/**
  * Deserializes the supplied (wire) buffer into suback data
  * @param packet_id returned integer - the MQTT packet identifier
  * @param max_count - the maximum number of members allowed in the grantedQoSs array
  * @param count returned integer - number of members in the grantedQoSs array
  * @param grantedQoSs returned array of integers - the granted qualities of service
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buf_len the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int deserialize_suback_packet(uint16_t *packet_id, uint32_t max_count, uint32_t *count,
                                     QoS *grantedQoSs, unsigned char *buf, size_t buf_len) 
{
    POINTER_VALID_CHECK(packet_id, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(count, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(grantedQoSs, ERR_PARAM_INVALID);

    unsigned char header, type = 0;
    unsigned char *curdata = buf;
    unsigned char *enddata = NULL;
    int decodeRc;
    uint32_t decodedLen = 0;
    uint32_t readBytesLen = 0;

    // SUBACK头部大小为4字节, 负载部分至少为1字节QOS返回码
    if (5 > buf_len) {
        return ERR_MQTT_BUFFER_TOO_SHORT;
    }
    // 读取报文固定头部的第一个字节
    header = mqtt_read_char(&curdata);
    type = (header&MQTT_HEADER_TYPE_MASK)>>MQTT_HEADER_TYPE_SHIFT;
    if (type != SUBACK) {
        return FAILURE_RET;
    }

    // 读取报文固定头部的剩余长度
    decodeRc = mqtt_read_packet_rem_len_form_buf(curdata, &decodedLen, &readBytesLen);
    if (decodeRc != SUCCESS_RET) {
        return decodeRc;
    }

    curdata += (readBytesLen);
    enddata = curdata + decodedLen;
    if (enddata - curdata < 2) {
        return FAILURE_RET;
    }

    // 读取报文可变头部的报文标识符
    *packet_id = mqtt_read_uint16_t(&curdata);

    // 读取报文的负载部分
    *count = 0;
    while (curdata < enddata) {
        if (*count > max_count) {
            return FAILURE_RET;
        }
        grantedQoSs[(*count)++] = (QoS) mqtt_read_char(&curdata);
    }

    return SUCCESS_RET;
}

/**
  * Deserializes the supplied (wire) buffer into unsuback data
  * @param packet_id returned integer - the MQTT packet identifier
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buf_len the length in bytes of the data in the supplied buffer
  * @return int indicating function execution status
  */
int deserialize_unsuback_packet(uint16_t *packet_id, unsigned char *buf, size_t buf_len) 
{
    POINTER_VALID_CHECK(buf, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(packet_id, ERR_PARAM_INVALID);

    unsigned char type = 0;
    unsigned char dup = 0;
    int ret;

    ret = deserialize_ack_packet(&type, &dup, packet_id, buf, buf_len);
    if (SUCCESS_RET == ret && UNSUBACK != type) {
        ret = FAILURE_RET;
    }

    return ret;
}

/**
  * Serializes a 0-length packet into the supplied buffer, ready for writing to a socket
  * @param buf the buffer into which the packet will be serialized
  * @param buf_len the length in bytes of the supplied buffer, to avoid overruns
  * @param packettype the message type
  * @param serialized length
  * @return int indicating function execution status
  */
int serialize_packet_with_zero_payload(unsigned char *buf, size_t buf_len, MessageTypes packetType, uint32_t *serialized_len) {
    POINTER_VALID_CHECK(buf, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(serialized_len, ERR_PARAM_INVALID);

    unsigned char header = 0;
    unsigned char *ptr = buf;
    int ret;

    /* Buffer should have at least 2 bytes for the header */
    if (4 > buf_len) {
        return ERR_MQTT_BUFFER_TOO_SHORT;
    }

    ret = mqtt_init_packet_header(&header, packetType, QOS0, 0, 0);
    if (SUCCESS_RET != ret) {
        return ret;
    }

    /* write header */
    mqtt_write_char(&ptr, header);

    /* write remaining length */
    ptr += mqtt_write_packet_rem_len(ptr, 0);
    *serialized_len = (uint32_t) (ptr - buf);

    return SUCCESS_RET;
}

int send_mqtt_packet(UIoT_Client *pClient, size_t length, Timer *timer) {
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(timer, ERR_PARAM_INVALID);

    size_t sent = 0;

    if (length >= pClient->write_buf_size) {
        return ERR_MQTT_BUFFER_TOO_SHORT;
    }

    while (sent < length && !has_expired(timer)) {
        int send_len = 0;
        send_len = pClient->network_stack.write(&(pClient->network_stack), &(pClient->write_buf[sent]), length, left_ms(timer));
        if (send_len < 0) {
            /* there was an error writing the data */
            break;
        }
        sent = sent + send_len;
    }

    if (sent == length) {
        /* record the fact that we have successfully sent the packet */
        //countdown(&c->ping_timer, c->keep_alive_interval);
        return SUCCESS_RET;
    }

    return FAILURE_RET;
}

/**
 * @brief 解析报文的剩余长度字段
 *
 * 每从网络中读取一个字节, 按照MQTT协议算法计算剩余长度
 *
 * @param pClient Client结构体
 * @param value   剩余长度
 * @param timeout 超时时间
 * @return
 */
static int _decode_packet_rem_len_with_net_read(UIoT_Client *pClient, uint32_t *value, uint32_t timeout) {
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(value, ERR_PARAM_INVALID);

    unsigned char i;
    uint32_t multiplier = 1;
    uint32_t len = 0;

    *value = 0;

    do {
        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES) {
            /* bad data */
            return ERR_MQTT_PACKET_READ_ERROR;
        }

        if (pClient->network_stack.read(&(pClient->network_stack), &i, 1, timeout) <= 0) {
            /* The value argument is the important value. len is just used temporarily
             * and never used by the calling function for anything else */
            return FAILURE_RET;
        }

        *value += ((i & 127) * multiplier);
        multiplier *= 128;
    } while ((i & 128) != 0);

    /* The value argument is the important value. len is just used temporarily
     * and never used by the calling function for anything else */
    return SUCCESS_RET;
}

/**
 * @brief 从底层SSL/TCP层读取报文数据
 *
 * 1. 读取第一个字节确定报文的类型;
 * 2. 读取剩余长度字段, 最大为四个字节; 剩余长度表示可变包头和有效负载的长度
 * 3. 根据剩余长度, 读取剩下的数据, 包括可变包头和有效负荷
 *
 * @param pClient        Client结构体
 * @param timer          定时器
 * @param packet_type    报文类型
 * @return
 */
static int _read_mqtt_packet(UIoT_Client *pClient, Timer *timer, uint8_t *packet_type) {
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(timer, ERR_PARAM_INVALID);

    uint32_t len = 0;
    uint32_t rem_len = 0;
    int read_len = 0;
    int ret;
    int timer_left_ms = left_ms(timer);
    
    if (timer_left_ms <= 0) {
        timer_left_ms = 1;
    }

    // 1. 读取报文固定头部的第一个字节
    read_len = pClient->network_stack.read(&(pClient->network_stack), pClient->read_buf, 1, timer_left_ms);
    if (read_len < 0) {
        return read_len;
    } else if (read_len == 0) {
        return ERR_MQTT_NOTHING_TO_READ;
    }

    len = 1;

    // 2. 读取报文固定头部剩余长度部分
    timer_left_ms = left_ms(timer);
    if (timer_left_ms <= 0) {
        timer_left_ms = 1;
    }
    timer_left_ms += UIOT_MQTT_MAX_REMAIN_WAIT_MS; //确保一包MQTT报文接收完
    
    ret = _decode_packet_rem_len_with_net_read(pClient, &rem_len, timer_left_ms);
    if (SUCCESS_RET != ret) {
        return ret;
    }

    // 如果读缓冲区的大小小于报文的剩余长度, 报文会被丢弃
    if (rem_len >= pClient->read_buf_size) {
        size_t total_bytes_read = 0;
        size_t bytes_to_be_read;

        timer_left_ms = left_ms(timer);
        if (timer_left_ms <= 0) {
            timer_left_ms = 1;
        }
        timer_left_ms += UIOT_MQTT_MAX_REMAIN_WAIT_MS;

        bytes_to_be_read = pClient->read_buf_size;
        do {
            read_len = pClient->network_stack.read(&(pClient->network_stack), pClient->read_buf, bytes_to_be_read, timer_left_ms);
            if (read_len > 0) {
                total_bytes_read += read_len;
                if ((rem_len - total_bytes_read) >= pClient->read_buf_size) {
                    bytes_to_be_read = pClient->read_buf_size;
                } else {
                    bytes_to_be_read = rem_len - total_bytes_read;
                }
            }
        } while (total_bytes_read < rem_len && read_len > 0);

        LOG_ERROR("MQTT Recv buffer not enough: %d < %d", pClient->read_buf_size, rem_len);
        return ERR_MQTT_BUFFER_TOO_SHORT;
    }

    // 将剩余长度写入读缓冲区
    len += mqtt_write_packet_rem_len(pClient->read_buf + 1, rem_len);

    // 3. 读取报文的剩余部分数据
    if (rem_len > 0 && ((len + rem_len) > pClient->read_buf_size)) {
        
        timer_left_ms = left_ms(timer);
        if (timer_left_ms <= 0) {
            timer_left_ms = 1;
        }
        timer_left_ms += UIOT_MQTT_MAX_REMAIN_WAIT_MS;
    
        pClient->network_stack.read(&(pClient->network_stack), pClient->read_buf, rem_len, timer_left_ms);
        return ERR_MQTT_BUFFER_TOO_SHORT;
    } else {
        if (rem_len > 0) {

            timer_left_ms = left_ms(timer);
            if (timer_left_ms <= 0) {
                timer_left_ms = 1;
            }
            timer_left_ms += UIOT_MQTT_MAX_REMAIN_WAIT_MS;
            read_len = pClient->network_stack.read(&(pClient->network_stack), pClient->read_buf + len, rem_len, timer_left_ms);
            if (read_len < 0) {
                return read_len;
            } else if (read_len == 0) {
                return ERR_MQTT_NOTHING_TO_READ;
            }
        }
    }

    *packet_type = (pClient->read_buf[0]&MQTT_HEADER_TYPE_MASK)>>MQTT_HEADER_TYPE_SHIFT;
    
    return SUCCESS_RET;
}

/**
 * @brief 消息主题是否相同
 *
 * @param topic_filter
 * @param topicName
 * @return
 */
static uint8_t _is_topic_equals(char *topic_filter, char *topicName) {
    return (uint8_t) (strlen(topic_filter) == strlen(topicName) && !strcmp(topic_filter, topicName));
}

/**
 * @brief 消息主题匹配
 *
 * assume topic filter and name is in correct format
 * # can only be at end
 * + and # can only be next to separator
 *
 * @param topic_filter   订阅消息的主题名
 * @param topicName     收到消息的主题名, 不能包含通配符
 * @param topicNameLen  主题名的长度
 * @return
 */
static uint8_t _is_topic_matched(char *topic_filter, char *topicName, uint16_t topicNameLen) {
    char *curf;
    char *curn;
    char *curn_end;

    curf = topic_filter;
    curn = topicName;
    curn_end = curn + topicNameLen;

    while (*curf && (curn < curn_end)) {

        if (*curf == '+' && *curn == '/') {
            curf++;
            continue;
        }

        if (*curn == '/' && *curf != '/') {
            break;
        }

        if (*curf != '+' && *curf != '#' && *curf != *curn) {
            break;
        }

        if (*curf == '+') {
            /* skip until we meet the next separator, or end of string */
            char *nextpos = curn + 1;
            while (nextpos < curn_end && *nextpos != '/')
                nextpos = ++curn + 1;
        } else if (*curf == '#') {
            /* skip until end of string */
            curn = curn_end - 1;
        }

        curf++;
        curn++;
    };

    if (*curf == '\0') {
        return (uint8_t) (curn == curn_end);
    } else {
        return (uint8_t) ((*curf == '#') || *(curf + 1) == '#' || (*curf == '+' && *(curn - 1) == '/'));
    }
}

/**
 * @brief 终端收到服务器的的PUBLISH消息之后, 传递消息给消息回调处理函数
 *
 * @param pClient
 * @param topicName
 * @param message
 * @return
 */
static int _deliver_message(UIoT_Client *pClient, char *topicName, uint16_t topicNameLen, MQTTMessage *message) {
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(topicName, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(message, ERR_PARAM_INVALID);

    message->topic = topicName;
    message->topic_len = (size_t)topicNameLen;

    uint32_t i;
    int flag_matched = 0;
    
    HAL_MutexLock(pClient->lock_generic);
    for (i = 0; i < MAX_SUB_TOPICS; ++i) {
        if ((pClient->sub_handles[i].topic_filter != NULL)
            && (_is_topic_equals(topicName, (char *) pClient->sub_handles[i].topic_filter) ||
                _is_topic_matched((char *) pClient->sub_handles[i].topic_filter, topicName, topicNameLen)))
        {
            HAL_MutexUnlock(pClient->lock_generic);
            if (pClient->sub_handles[i].message_handler != NULL) {
                pClient->sub_handles[i].message_handler(pClient, message, pClient->sub_handles[i].message_handler_data);
                return SUCCESS_RET;
            }
            HAL_MutexLock(pClient->lock_generic);
        }
    }

    /* Message handler not found for topic */
    /* May be we do not care  change FAILURE  use SUCCESS*/
    HAL_MutexUnlock(pClient->lock_generic);

    if (0 == flag_matched) {
        LOG_DEBUG("no matching any topic, call default handle function");

        if (NULL != pClient->event_handler.h_fp) {
            MQTTEventMsg msg;
            msg.event_type = MQTT_EVENT_PUBLISH_RECEIVED;
            msg.msg = message;
            pClient->event_handler.h_fp(pClient, pClient->event_handler.context, &msg);
        }
    }

    return SUCCESS_RET;
}

/**
 * @brief 从等待 publish ACK 的列表中，移除由 msdId 标记的元素
 *
 * @param c
 * @param msgId
 *
 * @return 0, success; NOT 0, fail;
 */
static int _mask_pubInfo_from(UIoT_Client *c, uint16_t msgId)
{
    if (!c) {
        return FAILURE_RET;
    }

    HAL_MutexLock(c->lock_list_pub);
    if (c->list_pub_wait_ack->len) {
        ListIterator *iter;
        ListNode *node = NULL;
        UIoTPubInfo *repubInfo = NULL;

        if (NULL == (iter = list_iterator_new(c->list_pub_wait_ack, LIST_TAIL))) {
            HAL_MutexUnlock(c->lock_list_pub);
            return SUCCESS_RET;
        }

        for (;;) {
            node = list_iterator_next(iter);

            if (NULL == node) {
                break;
            }

            repubInfo = (UIoTPubInfo *) node->val;
            if (NULL == repubInfo) {
                LOG_ERROR("node's value is invalid!");
                continue;
            }

            if (repubInfo->msg_id == msgId) {
                repubInfo->node_state = MQTT_NODE_STATE_INVALID; /* 标记为无效节点 */
            }
        }

        list_iterator_destroy(iter);
    }
    HAL_MutexUnlock(c->lock_list_pub);

    return SUCCESS_RET;
}

/* 从等待 subscribe(unsubscribe) ACK 的列表中，移除由 msdId 标记的元素 */
/* 同时返回消息处理数据 messageHandler */
/* return: 0, success; NOT 0, fail; */
static int _mask_sub_info_from(UIoT_Client *c, unsigned int msgId, SubTopicHandle *messageHandler)
{
    if (NULL == c || NULL == messageHandler) {
        return FAILURE_RET;
    }

    HAL_MutexLock(c->lock_list_sub);
    if (c->list_sub_wait_ack->len) {
        ListIterator *iter;
        ListNode *node = NULL;
        UIoTSubInfo *sub_info = NULL;

        if (NULL == (iter = list_iterator_new(c->list_sub_wait_ack, LIST_TAIL))) {
            HAL_MutexUnlock(c->lock_list_sub);
            return SUCCESS_RET;
        }

        for (;;) {
            node = list_iterator_next(iter);
            if (NULL == node) {
                break;
            }

            sub_info = (UIoTSubInfo *) node->val;
            if (NULL == sub_info) {
                LOG_ERROR("node's value is invalid!");
                continue;
            }

            if (sub_info->msg_id == msgId) {
                *messageHandler = sub_info->handler; /* return handle */
                sub_info->node_state = MQTT_NODE_STATE_INVALID; /* mark as invalid node */
            } 
        }

        list_iterator_destroy(iter);
    }
    HAL_MutexUnlock(c->lock_list_sub);

    return SUCCESS_RET;
}

/**
 * @brief 终端收到服务器的的PUBACK消息之后, 处理收到的PUBACK报文
 */
static int _handle_puback_packet(UIoT_Client *pClient, Timer *timer)
{
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(timer, ERR_PARAM_INVALID);

    uint16_t packet_id;
    uint8_t dup, type;
    int ret;

    ret = deserialize_ack_packet(&type, &dup, &packet_id, pClient->read_buf, pClient->read_buf_size);
    if (SUCCESS_RET != ret) {
        return ret;
    }

    (void)_mask_pubInfo_from(pClient, packet_id);

    /* 调用回调函数，通知外部PUBLISH成功. */
    if (NULL != pClient->event_handler.h_fp) {
        MQTTEventMsg msg;
        msg.event_type = MQTT_EVENT_PUBLISH_SUCCESS;
        msg.msg = (void *)(uintptr_t)packet_id;
        pClient->event_handler.h_fp(pClient, pClient->event_handler.context, &msg);
    }

    return SUCCESS_RET;
}

/**
 * @brief 终端收到服务器的的 SUBACK 消息之后, 处理收到的 SUBACK 报文
 */
static int _handle_suback_packet(UIoT_Client *pClient, Timer *timer, QoS qos)
{
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(timer, ERR_PARAM_INVALID);

    uint32_t count = 0;
    uint16_t packet_id = 0;
    QoS grantedQoS[3] = {QOS0, QOS0, QOS0};
    int ret;
    bool sub_nack = false;
    
    // 反序列化SUBACK报文
    ret = deserialize_suback_packet(&packet_id, 1, &count, grantedQoS, pClient->read_buf, pClient->read_buf_size);
    if (SUCCESS_RET != ret) {
        return ret;
    }

    int flag_dup = 0, i_free = -1;

    int j;
    for (j = 0; j <  count; j++) {
        /* In negative case, grantedQoS will be 0xFFFF FF80, which means -128 */
        if ((uint8_t)grantedQoS[j] == 0x80) {
            sub_nack = true;
            LOG_ERROR("MQTT SUBSCRIBE failed, ack code is 0x80");
        }
    }

    HAL_MutexLock(pClient->lock_generic);
    
    SubTopicHandle sub_handle;
    memset(&sub_handle, 0, sizeof(SubTopicHandle));
    (void)_mask_sub_info_from(pClient, (unsigned int)packet_id, &sub_handle);

    if (/*(NULL == sub_handle.message_handler) || */(NULL == sub_handle.topic_filter)) {
        LOG_ERROR("sub_handle is illegal, topic is null");
        HAL_MutexUnlock(pClient->lock_generic);
        return ERR_MQTT_SUB_FAILED;
    }

    if (sub_nack) {
        LOG_ERROR("MQTT SUBSCRIBE failed, packet_id: %u topic: %s", packet_id, sub_handle.topic_filter);
        /* 调用回调函数，通知外部 SUBSCRIBE 失败. */
        if (NULL != pClient->event_handler.h_fp) {
            MQTTEventMsg msg;
            msg.event_type = MQTT_EVENT_SUBSCRIBE_NACK;
            msg.msg = (void *)(uintptr_t)packet_id;
            if (pClient->event_handler.h_fp != NULL)
                pClient->event_handler.h_fp(pClient, pClient->event_handler.context, &msg);
        }
        HAL_Free((void *)sub_handle.topic_filter);
        sub_handle.topic_filter = NULL;
        HAL_MutexUnlock(pClient->lock_generic);
        return ERR_MQTT_SUB_FAILED;
    }

    int i;
    for (i = 0; i < MAX_SUB_TOPICS; ++i) {
        if ((NULL != pClient->sub_handles[i].topic_filter)) {
            if (0 == _check_handle_is_identical(&pClient->sub_handles[i], &sub_handle)) {
                flag_dup = 1;
                HAL_Free((void *)sub_handle.topic_filter);
                sub_handle.topic_filter = NULL;
                break;
            }
        } else {
            if (-1 == i_free) {
                i_free = i; /* record available element */
            }
        }
    }

    if (0 == flag_dup) {
        if (-1 == i_free) {
            LOG_ERROR("NO more @sub_handles space!");
            HAL_MutexUnlock(pClient->lock_generic);
            return FAILURE_RET;
        } else {
            pClient->sub_handles[i_free].topic_filter = sub_handle.topic_filter;
            pClient->sub_handles[i_free].message_handler = sub_handle.message_handler;
            pClient->sub_handles[i_free].qos = sub_handle.qos;
            pClient->sub_handles[i_free].message_handler_data = sub_handle.message_handler_data;
        }
    }
    
    HAL_MutexUnlock(pClient->lock_generic);

    /* 调用回调函数，通知外部 SUBSCRIBE 成功. */
    if (NULL != pClient->event_handler.h_fp) {
        MQTTEventMsg msg;
        msg.event_type = MQTT_EVENT_SUBSCRIBE_SUCCESS;
        msg.msg = (void *)(uintptr_t)packet_id;
        if (pClient->event_handler.h_fp != NULL)
            pClient->event_handler.h_fp(pClient, pClient->event_handler.context, &msg);
    }

    return SUCCESS_RET;
}

/**
 * @brief 终端收到服务器的的 USUBACK 消息之后, 处理收到的 USUBACK 报文
 */
static int _handle_unsuback_packet(UIoT_Client *pClient, Timer *timer)
{
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(timer, ERR_PARAM_INVALID);

    uint16_t packet_id = 0;

    int ret =  deserialize_unsuback_packet(&packet_id, pClient->read_buf, pClient->read_buf_size);
    if (ret != SUCCESS_RET) {
        return ret;
    }

    SubTopicHandle messageHandler;
    (void)_mask_sub_info_from(pClient, packet_id, &messageHandler);

    /* Remove from message handler array */
    HAL_MutexLock(pClient->lock_generic);

    /* actually below code is nonsense as unsub handle is different with sub handle even the same topic_filter*/
    #if 0
    int i;
    for (i = 0; i < MAX_SUB_TOPICS; ++i) {
        if ((pClient->sub_handles[i].topic_filter != NULL)
            && (0 == _check_handle_is_identical(&pClient->sub_handles[i], &messageHandler))) {            
            memset(&pClient->sub_handles[i], 0, sizeof(SubTopicHandle));

            /* NOTE: in case of more than one register(subscribe) with different callback function,
             *       so we must keep continuously searching related message handle. */
        }
    }
    #endif

    /* Free the topic filter malloced in uiot_mqtt_unsubscribe */
    if (messageHandler.topic_filter) {
        HAL_Free((void *)messageHandler.topic_filter);
        messageHandler.topic_filter = NULL;
    }

    if (NULL != pClient->event_handler.h_fp) {
        MQTTEventMsg msg;
        msg.event_type = MQTT_EVENT_UNSUBSCRIBE_SUCCESS;
        msg.msg = (void *)(uintptr_t)packet_id;

        pClient->event_handler.h_fp(pClient, pClient->event_handler.context, &msg);
    }

    HAL_MutexUnlock(pClient->lock_generic);

    return SUCCESS_RET;
}

#ifdef MQTT_CHECK_REPEAT_MSG

#define MQTT_MAX_REPEAT_BUF_LEN 50
static uint16_t sg_repeat_packet_id_buf[MQTT_MAX_REPEAT_BUF_LEN];

/**
 * @brief 判断packet_id缓存中是否已经存有传入的packet_id;
 */
static int _get_packet_id_in_repeat_buf(uint16_t packet_id)
{
    int i;
    for (i = 0; i < MQTT_MAX_REPEAT_BUF_LEN; ++i)
    {
        if (packet_id == sg_repeat_packet_id_buf[i])
        {
            return packet_id;
        }
    }
    return -1;
}

static void _add_packet_id_to_repeat_buf(uint16_t packet_id)
{
    static unsigned int current_packet_id_cnt = 0;
    if (_get_packet_id_in_repeat_buf(packet_id) > 0)
        return;

    sg_repeat_packet_id_buf[current_packet_id_cnt++] = packet_id;

    if (current_packet_id_cnt >= MQTT_MAX_REPEAT_BUF_LEN)
        current_packet_id_cnt = current_packet_id_cnt % 50;
}

void reset_repeat_packet_id_buffer(void)
{
    int i;
    for (i = 0; i < MQTT_MAX_REPEAT_BUF_LEN; ++i)
    {
        sg_repeat_packet_id_buf[i] = 0;
    }
}

#endif

/**
 * @brief 终端收到服务器的的PUBLISH消息之后, 处理收到的PUBLISH报文
 */
static int _handle_publish_packet(UIoT_Client *pClient, Timer *timer) {
    char *topic_name;
    uint16_t topic_len;
    MQTTMessage msg;
    int ret;
    uint32_t len = 0;

    ret = deserialize_publish_packet(&msg.dup, &msg.qos, &msg.retained, &msg.id, &topic_name, &topic_len, (unsigned char **) &msg.payload,
                                    &msg.payload_len, pClient->read_buf, pClient->read_buf_size);
    if (SUCCESS_RET != ret) {
        return ret;
    }
    
    // 传过来的topicName没有截断，会把payload也带过来
    char fix_topic[MAX_SIZE_OF_CLOUD_TOPIC] = {0};
    
    if(topic_len > MAX_SIZE_OF_CLOUD_TOPIC){
        topic_len = MAX_SIZE_OF_CLOUD_TOPIC - 1;
        LOG_ERROR("topic len exceed buffer len");
    }
    memcpy(fix_topic, topic_name, topic_len);

    if (QOS0 == msg.qos)
    {
        ret = _deliver_message(pClient, fix_topic, topic_len, &msg);
        if (SUCCESS_RET != ret)
            return ret;

        /* No further processing required for QOS0 */
        return ret;

    } else {
#ifdef MQTT_CHECK_REPEAT_MSG
        // 判断packet_id之前是否已经收到过
        int repeat_id = _get_packet_id_in_repeat_buf(msg.id);

        // 执行订阅消息的回调函数
        if (repeat_id < 0)
        {
#endif
            ret = _deliver_message(pClient, fix_topic, topic_len, &msg);
            if (SUCCESS_RET != ret)
                return ret;
#ifdef MQTT_CHECK_REPEAT_MSG
        }
        _add_packet_id_to_repeat_buf(msg.id);
#endif
    }
    
    HAL_MutexLock(pClient->lock_write_buf);
    if (QOS1 == msg.qos) {
        ret = serialize_pub_ack_packet(pClient->write_buf, pClient->write_buf_size, PUBACK, 0, msg.id, &len);
    } else { /* Message is not QOS0 or 1 means only option left is QOS2 */
        ret = serialize_pub_ack_packet(pClient->write_buf, pClient->write_buf_size, PUBREC, 0, msg.id, &len);
    }

    if (SUCCESS_RET != ret) {
        HAL_MutexUnlock(pClient->lock_write_buf);
        return ret;
    }

    ret = send_mqtt_packet(pClient, len, timer);
    if (SUCCESS_RET != ret) {
        HAL_MutexUnlock(pClient->lock_write_buf);
        return ret;
    }

    HAL_MutexUnlock(pClient->lock_write_buf);
    return SUCCESS_RET;
}

/**
 * @brief 处理PUBREC报文, 并发送PUBREL报文, PUBLISH报文为QOS2时
 *
 * @param pClient
 * @param timer
 * @return
 */
static int _handle_pubrec_packet(UIoT_Client *pClient, Timer *timer) {
    uint16_t packet_id;
    unsigned char dup, type;
    int ret;
    uint32_t len;

    ret = deserialize_ack_packet(&type, &dup, &packet_id, pClient->read_buf, pClient->read_buf_size);
    if (SUCCESS_RET != ret) {
        return ret;
    }

    HAL_MutexLock(pClient->lock_write_buf);
    ret = serialize_pub_ack_packet(pClient->write_buf, pClient->write_buf_size, PUBREL, 0, packet_id, &len);
    if (SUCCESS_RET != ret) {
        HAL_MutexUnlock(pClient->lock_write_buf);
        return ret;
    }

    /* send the PUBREL packet */
    ret = send_mqtt_packet(pClient, len, timer);
    if (SUCCESS_RET != ret) {
        HAL_MutexUnlock(pClient->lock_write_buf);
        /* there was a problem */
        return ret;
    }

    HAL_MutexUnlock(pClient->lock_write_buf);
    return SUCCESS_RET;
}

/**
 * @brief 处理服务器的心跳包回包
 *
 * @param pClient
 */
static void _handle_pingresp_packet(UIoT_Client *pClient) {
    HAL_MutexLock(pClient->lock_generic);
    pClient->is_ping_outstanding = 0;
    countdown(&pClient->ping_timer, pClient->options.keep_alive_interval);
    HAL_MutexUnlock(pClient->lock_generic);

    return;
}

int cycle_for_read(UIoT_Client *pClient, Timer *timer, uint8_t *packet_type, QoS qos) {
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(timer, ERR_PARAM_INVALID);

    int ret;
    /* read the socket, see what work is due */
    ret = _read_mqtt_packet(pClient, timer, packet_type);

    if (ERR_MQTT_NOTHING_TO_READ == ret) {
        /* Nothing to read, not a cycle failure */
        return SUCCESS_RET;
    }

    if (SUCCESS_RET != ret) {
        return ret;
    }

    switch (*packet_type) {
        case CONNACK:
            break;
        case PUBACK:
            ret = _handle_puback_packet(pClient, timer);
            break;
        case SUBACK:
            ret = _handle_suback_packet(pClient, timer, qos);
            break;
        case UNSUBACK:
            ret = _handle_unsuback_packet(pClient, timer);
            break;
        case PUBLISH: {
            ret = _handle_publish_packet(pClient, timer);
            break;
        }
        case PUBREC: {
            ret = _handle_pubrec_packet(pClient, timer);
            break;
        }
        case PUBREL: {
            LOG_ERROR("Packet type PUBREL is currently NOT handled!");
            break;
        }
        case PUBCOMP:
            break;
        case PINGRESP: 
            break;
        default: {
            /* Either unknown packet type or Failure occurred
             * Should not happen */
             
            return ERR_MQTT_RX_MESSAGE_PACKET_TYPE_INVALID;
        }
    }

    switch (*packet_type) {
        /* Recv below msgs are all considered as PING OK */
        case PUBACK:
        case SUBACK:
        case UNSUBACK:
        case PINGRESP: {
            _handle_pingresp_packet(pClient);
            break;
        }
        /* Recv downlink pub means link is OK but we still need to send PING request */
        case PUBLISH: {
            HAL_MutexLock(pClient->lock_generic);
            pClient->is_ping_outstanding = 0;
            HAL_MutexUnlock(pClient->lock_generic);
            break;
        }
    }

    return ret;
}

int wait_for_read(UIoT_Client *pClient, uint8_t packet_type, Timer *timer, QoS qos) {
    int ret;
    uint8_t read_packet_type = 0;

    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(timer, ERR_PARAM_INVALID);

    do {
        if (has_expired(timer)) {
            ret = ERR_MQTT_REQUEST_TIMEOUT;
            break;
        }
        ret = cycle_for_read(pClient, timer, &read_packet_type, qos);
    } while (SUCCESS_RET == ret && read_packet_type != packet_type );

    return ret;
}

void set_client_conn_state(UIoT_Client *pClient, uint8_t connected) {
    HAL_MutexLock(pClient->lock_generic);
    pClient->is_connected = connected;
    HAL_MutexUnlock(pClient->lock_generic);
}

uint8_t get_client_conn_state(UIoT_Client *pClient) {
    uint8_t is_connected = 0;
    HAL_MutexLock(pClient->lock_generic);
    is_connected = pClient->is_connected;
    HAL_MutexUnlock(pClient->lock_generic);
    return is_connected;
}

/*
 * @brief 向 subscribe(unsubscribe) ACK 等待列表中添加元素
 *
 *
 * return: 0, success; NOT 0, fail;
 */
int push_sub_info_to(UIoT_Client *c, int len, unsigned short msgId, MessageTypes type,
                                   SubTopicHandle *handler, ListNode **node)
{
    if (!c || !handler || !node) {
        return ERR_PARAM_INVALID;
    }

    HAL_MutexLock(c->lock_list_sub);

    if (c->list_sub_wait_ack->len >= MAX_SUB_TOPICS) {
        HAL_MutexUnlock(c->lock_list_sub);
        LOG_ERROR("number of sub_info more than max! size = %d", c->list_sub_wait_ack->len);
        return ERR_MQTT_MAX_SUBSCRIPTIONS_REACHED;
    }

    UIoTSubInfo *sub_info = (UIoTSubInfo *)HAL_Malloc(sizeof(
            UIoTSubInfo) + len);
    if (NULL == sub_info) {
        HAL_MutexUnlock(c->lock_list_sub);
        LOG_ERROR("malloc failed!");
        return FAILURE_RET;
    }
    
    sub_info->node_state = MQTT_NODE_STATE_NORMAL;
    sub_info->msg_id = msgId;
    sub_info->len = len;

    init_timer(&sub_info->sub_start_time);
    countdown_ms(&sub_info->sub_start_time, c->command_timeout_ms);

    sub_info->type = type;
    sub_info->handler = *handler;
    sub_info->buf = (unsigned char *)sub_info + sizeof(UIoTSubInfo);

    memcpy(sub_info->buf, c->write_buf, len);

    *node = list_node_new(sub_info);
    if (NULL == *node) {
        HAL_MutexUnlock(c->lock_list_sub);
        LOG_ERROR("list_node_new failed!");
        return FAILURE_RET;
    }

    list_rpush(c->list_sub_wait_ack, *node);

    HAL_MutexUnlock(c->lock_list_sub);

    return SUCCESS_RET;
}

bool parse_mqtt_payload_retcode_type(char *pJsonDoc, uint32_t *pRetCode) 
{
    FUNC_ENTRY;

    bool ret = false;

    char *ret_code = (char *)LITE_json_value_of(PASSWORD_REPLY_RETCODE, pJsonDoc);
    if (ret_code == NULL) FUNC_EXIT_RC(false);

    if (sscanf(ret_code, "%" SCNu32, pRetCode) != 1) 
    {
        LOG_ERROR("parse RetCode failed, errCode: %d\n", ERR_JSON_PARSE);
    }
    else {
        ret = true;
    }

    HAL_Free(ret_code);

    FUNC_EXIT_RC(ret);
}

bool parse_mqtt_state_request_id_type(char *pJsonDoc, char **pType)
{
    *pType = (char *)LITE_json_value_of(PASSWORD_REPLY_REQUEST_ID, pJsonDoc);
    return *pType == NULL ? false : true;
}

bool parse_mqtt_state_password_type(char *pJsonDoc, char **pType)
{
    *pType = (char *)LITE_json_value_of(PASSWORD_REPLY_PASSWORD, pJsonDoc);
    return *pType == NULL ? false : true;
}

#ifdef __cplusplus
}
#endif
