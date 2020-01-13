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
  * Determines the length of the MQTT unsubscribe packet that would be produced using the supplied parameters
  * @param count the number of topic filter strings in topicFilters
  * @param topicFilters the array of topic filter strings to be used in the publish
  * @return the length of buffer needed to contain the serialized version of the packet
  */
static uint32_t _get_unsubscribe_packet_rem_len(uint32_t count, char **topicFilters) {
    size_t i;
    size_t len = 2; /* packetid */

    for (i = 0; i < count; ++i) {
        len += 2 + strlen(*topicFilters + i); /* length + topic*/
    }

    return (uint32_t) len;
}

/**
  * Serializes the supplied unsubscribe data into the supplied buffer, ready for sending
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buf_len the length in bytes of the data in the supplied buffer
  * @param dup integer - the MQTT dup flag
  * @param packet_id integer - the MQTT packet identifier
  * @param count - number of members in the topicFilters array
  * @param topicFilters - array of topic filter names
  * @param serialized_len - the length of the serialized data
  * @return int indicating function execution status
  */
static int _serialize_unsubscribe_packet(unsigned char *buf, size_t buf_len,
                                         uint8_t dup, uint16_t packet_id,
                                         uint32_t count, char **topicFilters,
                                         uint32_t *serialized_len) {
    POINTER_VALID_CHECK(buf, ERR_PARAM_INVALID);
    POINTER_VALID_CHECK(serialized_len, ERR_PARAM_INVALID);

    unsigned char *ptr = buf;
    unsigned char header = 0;
    uint32_t rem_len = 0;
    uint32_t i = 0;
    int ret;

    rem_len = _get_unsubscribe_packet_rem_len(count, topicFilters);
    if (get_mqtt_packet_len(rem_len) > buf_len) {
        return ERR_MQTT_BUFFER_TOO_SHORT;
    }

    ret = mqtt_init_packet_header(&header, UNSUBSCRIBE, QOS1, dup, 0);
    if (SUCCESS_RET != ret) {
        return ret;
    }
    mqtt_write_char(&ptr, header); /* write header */

    ptr += mqtt_write_packet_rem_len(ptr, rem_len); /* write remaining length */

    mqtt_write_uint_16(&ptr, packet_id);

    for (i = 0; i < count; ++i) {
        mqtt_write_utf8_string(&ptr, *topicFilters + i);
    }

    *serialized_len = (uint32_t) (ptr - buf);

    return SUCCESS_RET;
}

int uiot_mqtt_unsubscribe(UIoT_Client *pClient, char *topicFilter) {
    int ret;

    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);
    STRING_PTR_VALID_CHECK(topicFilter, ERR_PARAM_INVALID);

    int i = 0;
    Timer timer;
    uint32_t len = 0;
    uint16_t packet_id = 0;
    bool suber_exists = false;

    ListNode *node = NULL;

    size_t topicLen = strlen(topicFilter);
    if (topicLen > MAX_SIZE_OF_CLOUD_TOPIC) {
        return ERR_MAX_TOPIC_LENGTH;
    }
    
    /* Remove from message handler array */
    HAL_MutexLock(pClient->lock_generic);
    for (i = 0; i < MAX_SUB_TOPICS; ++i) {        
        if ((pClient->sub_handles[i].topic_filter != NULL && !strcmp(pClient->sub_handles[i].topic_filter, topicFilter))
            || strstr(topicFilter,"/#") != NULL || strstr(topicFilter,"/+") != NULL) {
            /* Free the topic filter malloced in uiot_mqtt_subscribe */
            HAL_Free((void *)pClient->sub_handles[i].topic_filter);
            pClient->sub_handles[i].topic_filter = NULL;
            /* We don't want to break here, if the same topic is registered
             * with 2 callbacks. Unlikely scenario */
            suber_exists = true;
        }
    }
    HAL_MutexUnlock(pClient->lock_generic);

    if (suber_exists == false) {
        LOG_ERROR("subscription does not exists: %s", topicFilter);
        return ERR_MQTT_UNSUB_FAILED;
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
    packet_id = get_next_packet_id(pClient);
    ret = _serialize_unsubscribe_packet(pClient->write_buf, pClient->write_buf_size, 0, packet_id, 1, &topic_filter_stored,
                                       &len);
    if (SUCCESS_RET != ret) {
        HAL_MutexUnlock(pClient->lock_write_buf);
        HAL_Free(topic_filter_stored);
        return ret;
    }

    SubTopicHandle sub_handle;
    sub_handle.topic_filter = topic_filter_stored;
    sub_handle.message_handler = NULL;
    sub_handle.message_handler_data = NULL;

    ret = push_sub_info_to(pClient, len, (unsigned int)packet_id, UNSUBSCRIBE, &sub_handle, &node);
    if (SUCCESS_RET != ret) {
        LOG_ERROR("push publish into to pubInfolist failed: %d", ret);
        HAL_MutexUnlock(pClient->lock_write_buf);
        HAL_Free(topic_filter_stored);
        return ret;
    }

    /* send the unsubscribe packet */
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

#ifdef __cplusplus
}
#endif
