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

#include "mqtt_client.h"

/**
 * @param mqttstring the MQTTString structure into which the data is to be read
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param enddata pointer to the end of the data: do not read beyond
 * @return SUCCESS if successful, FAILURE if not
 */
static int _read_string_with_len(char **string, uint16_t *stringLen, unsigned char **pptr, unsigned char *enddata) {
    int ret = FAILURE_RET;

    /* the first two bytes are the length of the string */
    /* enough length to read the integer? */
    if (enddata - (*pptr) > 1) {
        *stringLen = mqtt_read_uint16_t(pptr); /* increments pptr to point past length */
        
        if(*stringLen > UIOT_MQTT_RX_BUF_LEN){
            LOG_ERROR("stringLen exceed UIOT_MQTT_RX_BUF_LEN");
            return FAILURE_RET;
        }
        
        if (&(*pptr)[*stringLen] <= enddata) {
            *string = (char *) *pptr;
            *pptr += *stringLen;
            return SUCCESS_RET;
        }
    }

    return ret;
}

/**
  * Determines the length of the MQTT publish packet that would be produced using the supplied parameters
  * @param qos the MQTT QoS of the publish (packetid is omitted for QoS 0)
  * @param topicName the topic name to be used in the publish  
  * @param payload_len the length of the payload to be sent
  * @return the length of buffer needed to contain the serialized version of the packet
  */
static uint32_t _get_publish_packet_len(uint8_t qos, char *topicName, size_t payload_len) {
    size_t len = 0;

    len += 2 + strlen(topicName) + payload_len;
    if (qos > 0) {
        len += 2; /* packetid */
    }
    return (uint32_t) len;
}

static int _mask_push_pubInfo_to(UIoT_Client *c, int len, unsigned short msgId, ListNode **node)
{
    if (!c || !node) {
        LOG_ERROR("invalid parameters!");
        return ERR_MQTT_PUSH_TO_LIST_FAILED;
    }

    if ((len < 0) || (len > c->write_buf_size)) {
        LOG_ERROR("the param of len is error!");
        return FAILURE_RET;
    }

    HAL_MutexLock(c->lock_list_pub);

    if (c->list_pub_wait_ack->len >= MAX_REPUB_NUM) {
        HAL_MutexUnlock(c->lock_list_pub);
        LOG_ERROR("more than %u elements in republish list. List overflow!", c->list_pub_wait_ack->len);
        return FAILURE_RET;
    }

    UIoTPubInfo *repubInfo = (UIoTPubInfo *)HAL_Malloc(sizeof(UIoTPubInfo) + len);
    if (NULL == repubInfo) {
        HAL_MutexUnlock(c->lock_list_pub);
        LOG_ERROR("memory malloc failed!");
        return FAILURE_RET;
    }

    repubInfo->node_state = MQTT_NODE_STATE_NORMAL;
    repubInfo->msg_id = msgId;
    repubInfo->len = len;
    init_timer(&repubInfo->pub_start_time);
    countdown_ms(&repubInfo->pub_start_time, c->command_timeout_ms);

    repubInfo->buf = (unsigned char *)repubInfo + sizeof(UIoTPubInfo);

    memcpy(repubInfo->buf, c->write_buf, len);

    *node = list_node_new(repubInfo);
    if (NULL == *node) {
        HAL_MutexUnlock(c->lock_list_pub);
        LOG_ERROR("list_node_new failed!");
        return FAILURE_RET;
    }

    list_rpush(c->list_pub_wait_ack, *node);

    HAL_MutexUnlock(c->lock_list_pub);

    return SUCCESS_RET;
}

/**
  * Deserializes the supplied (wire) buffer into publish data
  * @param dup returned integer - the MQTT dup flag
  * @param qos returned integer - the MQTT QoS value
  * @param retained returned integer - the MQTT retained flag
  * @param packet_id returned integer - the MQTT packet identifier
  * @param topicName returned MQTTString - the MQTT topic in the publish
  * @param payload returned byte buffer - the MQTT publish payload
  * @param payload_len returned integer - the length of the MQTT payload
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buf_len the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int deserialize_publish_packet(uint8_t *dup, QoS *qos, uint8_t *retained, uint16_t *packet_id, char **topicName,
                               uint16_t *topicNameLen,unsigned char **payload, size_t *payload_len, unsigned char *buf, size_t buf_len) 
{
    POINTER_VALID_CHECK(dup, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(qos, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(retained, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(packet_id, ERR_PARAM_INVALID);

    unsigned char header, type = 0;
    unsigned char *curdata = buf;
    unsigned char *enddata = NULL;
    int ret;
    uint32_t decodedLen = 0;
    uint32_t readBytesLen = 0;

    /* Publish header size is at least four bytes.
     * Fixed header is two bytes.
     * Variable header size depends on QoS And Topic Name.
     * QoS level 0 doesn't have a message identifier (0 - 2 bytes)
     * Topic Name length fields decide size of topic name field (at least 2 bytes)
     * MQTT v3.1.1 Specification 3.3.1 */
    if (4 > buf_len) {
        return ERR_MQTT_BUFFER_TOO_SHORT;
    }

    header = mqtt_read_char(&curdata);
    type = (header&MQTT_HEADER_TYPE_MASK)>>MQTT_HEADER_TYPE_SHIFT;
    *dup  = (header&MQTT_HEADER_DUP_MASK)>>MQTT_HEADER_DUP_SHIFT;
    *qos  = (QoS)((header&MQTT_HEADER_QOS_MASK)>>MQTT_HEADER_QOS_SHIFT);
    *retained  = header&MQTT_HEADER_RETAIN_MASK;
        
    if (PUBLISH != type) {
        return FAILURE_RET;
    }

    /* read remaining length */
    ret = mqtt_read_packet_rem_len_form_buf(curdata, &decodedLen, &readBytesLen);
    if (SUCCESS_RET != ret) {
        return ret;
    }
    curdata += (readBytesLen);
    enddata = curdata + decodedLen;

    /* do we have enough data to read the protocol version byte? */
    if (SUCCESS_RET != _read_string_with_len(topicName, topicNameLen, &curdata, enddata) || (0 > (enddata - curdata))) {
        return FAILURE_RET;
    }

    if (QOS0 != *qos) {
        *packet_id = mqtt_read_uint16_t(&curdata);
    }

    *payload_len = (size_t) (enddata - curdata);
    *payload = curdata;

    return SUCCESS_RET;
}

/**
  * Serializes the ack packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param buf_len the length in bytes of the supplied buffer
  * @param packet_type the MQTT packet type: 1.PUBACK; 2.PUBREL; 3.PUBCOMP
  * @param dup the MQTT dup flag
  * @param packet_id the MQTT packet identifier
  * @return serialized length, or error if 0
  */
int serialize_pub_ack_packet(unsigned char *buf, size_t buf_len, MessageTypes packet_type, uint8_t dup,
                             uint16_t packet_id,
                             uint32_t *serialized_len) {
    POINTER_VALID_CHECK(buf, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(serialized_len, ERR_PARAM_INVALID);

    unsigned char header = 0;
    unsigned char *ptr = buf;
    QoS requestQoS = (PUBREL == packet_type) ? QOS1 : QOS0;  // 详见 MQTT协议说明 3.6.1小结
    int ret = mqtt_init_packet_header(&header, packet_type, requestQoS, dup, 0);

    /* Minimum byte length required by ACK headers is
     * 2 for fixed and 2 for variable part */
    if (4 > buf_len) {
        return ERR_MQTT_BUFFER_TOO_SHORT;
    }

    if (SUCCESS_RET != ret) {
        return ret;
    }
    mqtt_write_char(&ptr, header); /* write header */

    ptr += mqtt_write_packet_rem_len(ptr, 2); /* write remaining length */
    mqtt_write_uint_16(&ptr, packet_id);
    *serialized_len = (uint32_t) (ptr - buf);

    return SUCCESS_RET;
}


/**
  * Serializes the supplied publish data into the supplied buffer, ready for sending
  * @param buf the buffer into which the packet will be serialized
  * @param buf_len the length in bytes of the supplied buffer
  * @param dup integer - the MQTT dup flag
  * @param qos integer - the MQTT QoS value
  * @param retained integer - the MQTT retained flag
  * @param packet_id integer - the MQTT packet identifier
  * @param topicName MQTTString - the MQTT topic in the publish
  * @param payload byte buffer - the MQTT publish payload
  * @param payload_len integer - the length of the MQTT payload
  * @return the length of the serialized data.  <= 0 indicates error
  */
static int _serialize_publish_packet(unsigned char *buf, size_t buf_len, uint8_t dup, QoS qos, uint8_t retained,
                                     uint16_t packet_id,
                                     char *topicName, unsigned char *payload, size_t payload_len,
                                     uint32_t *serialized_len) {
    POINTER_VALID_CHECK(buf, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(serialized_len, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(payload, ERR_PARAM_INVALID);

    unsigned char *ptr = buf;
    unsigned char header = 0;
    uint32_t rem_len = 0;
    int ret;

    rem_len = _get_publish_packet_len(qos, topicName, payload_len);
    if (get_mqtt_packet_len(rem_len) > buf_len) {
        return ERR_MQTT_BUFFER_TOO_SHORT;
    }

    ret = mqtt_init_packet_header(&header, PUBLISH, qos, dup, retained);
    if (SUCCESS_RET != ret) {
        return ret;
    }

    mqtt_write_char(&ptr, header); /* write header */

    ptr += mqtt_write_packet_rem_len(ptr, rem_len); /* write remaining length */;

    mqtt_write_utf8_string(&ptr, topicName);   /* Variable Header: Topic Name */

    if (qos > 0) {
        mqtt_write_uint_16(&ptr, packet_id);  /* Variable Header: Topic Name */
    }

    memcpy(ptr, payload, payload_len);
    ptr += payload_len;

    *serialized_len = (uint32_t) (ptr - buf);

    return SUCCESS_RET;
}

int uiot_mqtt_publish(UIoT_Client *pClient, char *topicName, PublishParams *pParams) {
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pParams, ERR_PARAM_INVALID);
    STRING_PTR_VALID_CHECK(topicName, ERR_PARAM_INVALID);

    Timer timer;
    uint32_t len = 0;
    int ret;

    ListNode *node = NULL;
    
    size_t topicLen = strlen(topicName);
    if (topicLen > MAX_SIZE_OF_CLOUD_TOPIC) {
        return ERR_MAX_TOPIC_LENGTH;
    }

    if (pParams->qos == QOS2) {
        LOG_ERROR("QoS2 is not supported currently");
        return ERR_MQTT_QOS_NOT_SUPPORT;
    }

    if (!get_client_conn_state(pClient)) {
        return ERR_MQTT_NO_CONN;
    }

    init_timer(&timer);
    countdown_ms(&timer, pClient->command_timeout_ms);

    HAL_MutexLock(pClient->lock_write_buf);
    if (pParams->qos == QOS1) {
        pParams->id = get_next_packet_id(pClient);
        LOG_INFO("publish qos1 seq=%d|topicName=%s|payload=%s", pParams->id, topicName, (char *)pParams->payload);
    }
    else {
        LOG_INFO("publish qos0 seq=%d|topicName=%s|payload=%s", pParams->id, topicName, (char *)pParams->payload);
    }
    ret = _serialize_publish_packet(pClient->write_buf, pClient->write_buf_size, 0, pParams->qos, pParams->retained, pParams->id,
                                   topicName, (unsigned char *) pParams->payload, pParams->payload_len, &len);
    if (SUCCESS_RET != ret) {
        HAL_MutexUnlock(pClient->lock_write_buf);
        return ret;
    }

    if (pParams->qos > QOS0) {
        ret = _mask_push_pubInfo_to(pClient, len, pParams->id, &node);
        if (SUCCESS_RET != ret) {
            LOG_ERROR("push publish into pubInfolist failed!");
            HAL_MutexUnlock(pClient->lock_write_buf);
            return ret;
        }
    }

    /* send the publish packet */
    ret = send_mqtt_packet(pClient, len, &timer);
    if (SUCCESS_RET != ret) {
        if (pParams->qos > QOS0) {
            HAL_MutexLock(pClient->lock_list_pub);
            list_remove(pClient->list_pub_wait_ack, node);
            HAL_MutexUnlock(pClient->lock_list_pub);
        }

        HAL_MutexUnlock(pClient->lock_write_buf);
        return ret;
    }

    HAL_MutexUnlock(pClient->lock_write_buf);

    return pParams->id;
}

#ifdef __cplusplus
}
#endif
