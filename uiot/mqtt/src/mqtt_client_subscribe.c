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
  * Determines the length of the MQTT subscribe packet that would be produced using the supplied parameters
  * @param count the number of topic filter strings in topicFilters
  * @param topicFilters the array of topic filter strings to be used in the publish
  * @return the length of buffer needed to contain the serialized version of the packet
  */
static uint32_t _get_subscribe_packet_rem_len(uint32_t count, char **topicFilters) {
    size_t i;
    size_t len = 2; /* packetid */

    for (i = 0; i < count; ++i) {
        len += 2 + strlen(*topicFilters + i) + 1; /* length + topic + req_qos */
    }

    return (uint32_t) len;
}

/**
  * Serializes the supplied subscribe data into the supplied buffer, ready for sending
  * @param buf the buffer into which the packet will be serialized
  * @param buf_len the length in bytes of the supplied buffer
  * @param dup integer - the MQTT dup flag
  * @param packet_id integer - the MQTT packet identifier
  * @param count - number of members in the topicFilters and reqQos arrays
  * @param topicFilters - array of topic filter names
  * @param requestedQoSs - array of requested QoS
  * @return the length of the serialized data.  <= 0 indicates error
  */
static int _serialize_subscribe_packet(unsigned char *buf, size_t buf_len, uint8_t dup, uint16_t packet_id, uint32_t count,
                                char **topicFilters, QoS *requestedQoSs, uint32_t *serialized_len) {
    POINTER_VALID_CHECK(buf, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(serialized_len, ERR_PARAM_INVALID);

    unsigned char *ptr = buf;
    unsigned char header = 0;
    uint32_t rem_len = 0;
    uint32_t i = 0;
    int ret;

    // SUBSCRIBE报文的剩余长度 = 报文标识符(2 byte) + count * (长度字段(2 byte) + topicLen + qos(1 byte))
    rem_len = _get_subscribe_packet_rem_len(count, topicFilters);
    if (get_mqtt_packet_len(rem_len) > buf_len) {
        return ERR_MQTT_BUFFER_TOO_SHORT;
    }
    // 初始化报文头部
    ret = mqtt_init_packet_header(&header, SUBSCRIBE, QOS1, dup, 0);
    if (SUCCESS_RET != ret) {
        return ret;
    }
    // 写报文固定头部第一个字节
    mqtt_write_char(&ptr, header);
    // 写报文固定头部剩余长度字段
    ptr += mqtt_write_packet_rem_len(ptr, rem_len);
    // 写可变头部: 报文标识符
    mqtt_write_uint_16(&ptr, packet_id);
    // 写报文的负载部分数据
    for (i = 0; i < count; ++i) {
        mqtt_write_utf8_string(&ptr, *topicFilters + i);
        mqtt_write_char(&ptr, (unsigned char) requestedQoSs[i]);
    }

    *serialized_len = (uint32_t) (ptr - buf);

    return SUCCESS_RET;
}

int uiot_mqtt_subscribe(UIoT_Client *pClient, char *topicFilter, SubscribeParams *pParams) {
    int ret;

    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(pParams, ERR_PARAM_INVALID);
    STRING_PTR_VALID_CHECK(topicFilter, ERR_PARAM_INVALID);

    Timer timer;
    uint32_t len = 0;
    uint16_t packet_id = 0;

    ListNode *node = NULL;
    
    size_t topicLen = strlen(topicFilter);
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

    /* topic filter should be valid in the whole sub life */
    char *topic_filter_stored = HAL_Malloc(topicLen + 1);
    if (topic_filter_stored == NULL) {
        LOG_ERROR("malloc failed");
        return FAILURE_RET;
    }
    
    strcpy(topic_filter_stored, topicFilter);
    topic_filter_stored[topicLen] = 0;
    
    init_timer(&timer);
    countdown_ms(&timer, pClient->command_timeout_ms);

    HAL_MutexLock(pClient->lock_write_buf);
    // 序列化SUBSCRIBE报文
    packet_id = get_next_packet_id(pClient);
    LOG_DEBUG("topicName=%s|packet_id=%d|Userdata=%s\n", topic_filter_stored, packet_id, (char *)pParams->user_data);

    ret = _serialize_subscribe_packet(pClient->write_buf, pClient->write_buf_size, 0, packet_id, 1, &topic_filter_stored,
                                     &pParams->qos, &len);
    if (SUCCESS_RET != ret) {
        HAL_MutexUnlock(pClient->lock_write_buf);
        HAL_Free(topic_filter_stored);
        return ret;
    }

    /* 等待 sub ack 列表中添加元素 */
    SubTopicHandle sub_handle;
    sub_handle.topic_filter = topic_filter_stored;
    sub_handle.message_handler = pParams->on_message_handler;
    sub_handle.qos = pParams->qos;
    sub_handle.message_handler_data = pParams->user_data;

    ret = push_sub_info_to(pClient, len, (unsigned int)packet_id, SUBSCRIBE, &sub_handle, &node);
    if (SUCCESS_RET != ret) {
        LOG_ERROR("push publish into to pubInfolist failed!");
        HAL_MutexUnlock(pClient->lock_write_buf);
        HAL_Free(topic_filter_stored);
        return ret;
    }
    
    // 发送SUBSCRIBE报文
    ret = send_mqtt_packet(pClient, len, &timer);
    if (SUCCESS_RET != ret) {
        HAL_MutexLock(pClient->lock_list_sub);
        list_remove(pClient->list_sub_wait_ack, node);
        HAL_MutexUnlock(pClient->lock_list_sub);

        HAL_MutexUnlock(pClient->lock_write_buf);
        HAL_Free(topic_filter_stored);
        return ret;
    }

    HAL_MutexUnlock(pClient->lock_write_buf);

    return packet_id;
}

int uiot_mqtt_resubscribe(UIoT_Client *pClient) {
    int ret;

    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);

    uint32_t itr = 0;
    char *topic = NULL;
    SubscribeParams temp_param;

    if (!get_client_conn_state(pClient)) {
        return ERR_MQTT_NO_CONN;
    }

    for (itr = 0; itr < MAX_SUB_TOPICS; itr++) {
        topic = (char *) pClient->sub_handles[itr].topic_filter;
        if (topic == NULL) {
            continue;
        }
        temp_param.on_message_handler = pClient->sub_handles[itr].message_handler;
        temp_param.qos = pClient->sub_handles[itr].qos;
        temp_param.user_data = pClient->sub_handles[itr].message_handler_data;

        ret = uiot_mqtt_subscribe(pClient, topic, &temp_param);
        if (ret < 0) {
            LOG_ERROR("resubscribe failed %d, topic: %s", ret, topic);
            return ret;
        }
    }

    return SUCCESS_RET;
}

#ifdef __cplusplus
}
#endif
