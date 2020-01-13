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

#include "mqtt_client.h"

#define MQTT_PING_RETRY_TIMES   2

static void _iot_disconnect_callback(UIoT_Client *pClient)
{
    if (NULL != pClient->event_handler.h_fp) {
        MQTTEventMsg msg;
        msg.event_type = MQTT_EVENT_DISCONNECT;
        msg.msg = NULL;

        pClient->event_handler.h_fp(pClient, pClient->event_handler.context, &msg);
    }
}

static void _reconnect_callback(UIoT_Client* pClient) 
{
    if (NULL != pClient->event_handler.h_fp) {
        MQTTEventMsg msg;
        msg.event_type = MQTT_EVENT_RECONNECT;
        msg.msg = NULL;

        pClient->event_handler.h_fp(pClient, pClient->event_handler.context, &msg);
    }
}

/**
 * @brief 处理非手动断开连接的情况
 *
 * @param pClient
 * @return
 */
static int _handle_disconnect(UIoT_Client *pClient) {
    int ret;
    
    if (0 == get_client_conn_state(pClient)) {
        return ERR_MQTT_NO_CONN;
    }

    ret = uiot_mqtt_disconnect(pClient);
    // 若断开连接失败, 强制断开底层TLS层连接
    if (ret != SUCCESS_RET) {
        pClient->network_stack.disconnect(&(pClient->network_stack));
        set_client_conn_state(pClient, DISCONNECTED);
    }

    LOG_ERROR("disconnect MQTT for some reasons..");
    
    _iot_disconnect_callback(pClient);

    // 非手动断开连接
    pClient->was_manually_disconnected = 0;
    return ERR_MQTT_NO_CONN;
}

/**
 * @brief 处理自动重连的相关逻辑
 *
 * @param pClient
 * @return
 */
static int _handle_reconnect(UIoT_Client *pClient) {
    int ret = MQTT_RECONNECTED;

    // 自动重连等待时间还未过期, 还未到重连的时候, 返回正在进行重连
    if (!has_expired(&(pClient->reconnect_delay_timer))) {
        return ERR_MQTT_ATTEMPTING_RECONNECT;
    }

    ret = uiot_mqtt_attempt_reconnect(pClient);
    if (ret == MQTT_RECONNECTED) {
        LOG_ERROR("attempt to reconnect success.");
        _reconnect_callback(pClient);
        return ret;
    }
    else {
        LOG_ERROR("attempt to reconnect failed, errCode: %d", ret);
        ret = ERR_MQTT_ATTEMPTING_RECONNECT;
    }

    pClient->current_reconnect_wait_interval *= 2;

    if (MAX_RECONNECT_WAIT_INTERVAL < pClient->current_reconnect_wait_interval) {
        if (ENABLE_INFINITE_RECONNECT) {
            pClient->current_reconnect_wait_interval = MAX_RECONNECT_WAIT_INTERVAL;
        } else {
            return ERR_MQTT_RECONNECT_TIMEOUT;
        }
    }
    countdown_ms(&(pClient->reconnect_delay_timer), pClient->current_reconnect_wait_interval);

    return ret;
}

/**
 * @brief 处理与服务器维持心跳的相关逻辑
 *
 * @param pClient
 * @return
 */
static int _mqtt_keep_alive(UIoT_Client *pClient)
{
    int ret;
    Timer timer;
    uint32_t serialized_len = 0;

    if (0 == pClient->options.keep_alive_interval) {
        return SUCCESS_RET;
    }

    if (!has_expired(&pClient->ping_timer)) {
        return SUCCESS_RET;
    }

    if (pClient->is_ping_outstanding >= MQTT_PING_RETRY_TIMES) {
        //Reaching here means we haven't received any MQTT packet for a long time (keep_alive_interval)
        LOG_ERROR("Fail to recv MQTT msg. Something wrong with the connection.");
        ret = _handle_disconnect(pClient);
        return ret;
    }

    /* there is no ping outstanding - send one */    
    HAL_MutexLock(pClient->lock_write_buf);
    ret = serialize_packet_with_zero_payload(pClient->write_buf, pClient->write_buf_size, PINGREQ, &serialized_len);
    if (SUCCESS_RET != ret) {
        HAL_MutexUnlock(pClient->lock_write_buf);
        return ret;
    }

    /* send the ping packet */
    int i = 0;
    init_timer(&timer);    
    do {
        countdown_ms(&timer, pClient->command_timeout_ms);
        ret = send_mqtt_packet(pClient, serialized_len, &timer);
    } while (SUCCESS_RET != ret && (i++ < 3));
    
    if (SUCCESS_RET != ret) {
        HAL_MutexUnlock(pClient->lock_write_buf);        
        //If sending a PING fails, probably the connection is not OK and we decide to disconnect and begin reconnection attempts
        LOG_ERROR("Fail to send PING request. Something wrong with the connection.");
        ret = _handle_disconnect(pClient);
        return ret;
    }
    HAL_MutexUnlock(pClient->lock_write_buf);
    
    HAL_MutexLock(pClient->lock_generic);
    pClient->is_ping_outstanding++;
    /* start a timer to wait for PINGRESP from server */
    countdown(&pClient->ping_timer, Min(5, pClient->options.keep_alive_interval/2));
    HAL_MutexUnlock(pClient->lock_generic);
    LOG_DEBUG("PING request %u has been sent...", pClient->is_ping_outstanding);

    return SUCCESS_RET;
}

int uiot_mqtt_yield(UIoT_Client *pClient, uint32_t timeout_ms) {
    int ret = SUCCESS_RET;
    Timer timer;
    uint8_t packet_type;

    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);
    NUMERIC_VALID_CHECK(timeout_ms, ERR_PARAM_INVALID);

    // 1. 检查连接是否已经手动断开
    if (!get_client_conn_state(pClient) && pClient->was_manually_disconnected == 1) {
        return MQTT_MANUALLY_DISCONNECTED;
    }

    // 2. 检查连接是否断开, 自动连接是否开启
    if (!get_client_conn_state(pClient) && pClient->options.auto_connect_enable == 0) {
        return ERR_MQTT_NO_CONN;
    }

    init_timer(&timer);
    countdown_ms(&timer, timeout_ms);

    // 3. 循环读取消息以及心跳包管理
    while (!has_expired(&timer)) {
        if (!get_client_conn_state(pClient)) {
            if (pClient->current_reconnect_wait_interval > MAX_RECONNECT_WAIT_INTERVAL) {
                ret = ERR_MQTT_RECONNECT_TIMEOUT;
                break;
            }
            ret = _handle_reconnect(pClient);

            continue;
        }        

        //防止任务独占CPU时间过长
        HAL_SleepMs(10);

        ret = cycle_for_read(pClient, &timer, &packet_type, 0);

        if (ret == SUCCESS_RET) {
            /* check list of wait publish ACK to remove node that is ACKED or timeout */
            uiot_mqtt_pub_info_proc(pClient);

            /* check list of wait subscribe(or unsubscribe) ACK to remove node that is ACKED or timeout */
            uiot_mqtt_sub_info_proc(pClient);

            ret = _mqtt_keep_alive(pClient);
        }          
        else if (ret == ERR_SSL_READ_TIMEOUT || ret == ERR_SSL_READ_FAILED ||
                 ret == ERR_TCP_PEER_SHUTDOWN || ret == ERR_TCP_READ_FAILED){
            LOG_ERROR("network read failed, ret: %d. MQTT Disconnect.", ret);
            ret = _handle_disconnect(pClient);
        }
        else if(ret == FAILURE_RET) //最后一次读肯定失败,因为没有数据了
        {
            ret = SUCCESS_RET;
        }

        if (ret == ERR_MQTT_NO_CONN) {
            pClient->counter_network_disconnected++;

            if (pClient->options.auto_connect_enable == 1) {
                pClient->current_reconnect_wait_interval = MIN_RECONNECT_WAIT_INTERVAL;
                countdown_ms(&(pClient->reconnect_delay_timer), pClient->current_reconnect_wait_interval);

                // 如果超时时间到了,则会直接返回
                ret = ERR_MQTT_ATTEMPTING_RECONNECT;
            } else {
                break;
            }
        } else if (ret != SUCCESS_RET) {
            break;
        }
        
    }

    return ret;
}

/**
 * @brief puback等待超时检测
 *
 * @param pClient MQTTClient对象
 *
 */
int uiot_mqtt_pub_info_proc(UIoT_Client *pClient)
{
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);

    HAL_MutexLock(pClient->lock_list_pub);
    do {
        if (0 == pClient->list_pub_wait_ack->len) {
            break;
        }

        ListIterator *iter;
        ListNode *node = NULL;
        ListNode *temp_node = NULL;

        if (NULL == (iter = list_iterator_new(pClient->list_pub_wait_ack, LIST_TAIL))) {
            LOG_ERROR("new list failed");
            break;
        }

        for (;;) {
            node = list_iterator_next(iter);

            if (NULL != temp_node) {
                list_remove(pClient->list_pub_wait_ack, temp_node);
                temp_node = NULL;
            }

            if (NULL == node) {
                break; /* end of list */
            }

            UIoTPubInfo *repubInfo = (UIoTPubInfo *) node->val;
            if (NULL == repubInfo) {
                LOG_ERROR("node's value is invalid!");
                temp_node = node;
                continue;
            }

            /* remove invalid node */
            if (MQTT_NODE_STATE_INVALID == repubInfo->node_state) {
                temp_node = node;
                continue;
            }

            if (!pClient->is_connected) {
                continue;
            }

            /* check the request if timeout or not */
            if (left_ms(&repubInfo->pub_start_time) > 0) {
                continue;
            }

            /* If wait ACK timeout, republish */
            HAL_MutexUnlock(pClient->lock_list_pub);
            /* 重发机制交给上层用户二次开发, 这里先把超时的节点从列表中移除 */
            temp_node = node;

            countdown_ms(&repubInfo->pub_start_time, pClient->command_timeout_ms);
            HAL_MutexLock(pClient->lock_list_pub);

                /* 通知外部网络已经断开 */
            if (NULL != pClient->event_handler.h_fp) {
                MQTTEventMsg msg;
                msg.event_type = MQTT_EVENT_PUBLISH_TIMEOUT;
                msg.msg = (void *)(uintptr_t)repubInfo->msg_id;
                pClient->event_handler.h_fp(pClient, pClient->event_handler.context, &msg);
            }
        }

        list_iterator_destroy(iter);

    } while (0);

    HAL_MutexUnlock(pClient->lock_list_pub);

    return SUCCESS_RET;
}

/**
 * @brief suback等待超时检测
 *
 * @param pClient MQTTClient对象
 *
 */
int uiot_mqtt_sub_info_proc(UIoT_Client *pClient)
{
    int ret = SUCCESS_RET;

    if (!pClient) {
        return ERR_PARAM_INVALID;
    }

    HAL_MutexLock(pClient->lock_list_sub);
    do {
        if (0 == pClient->list_sub_wait_ack->len) {
            break;
        }

        ListIterator *iter;
        ListNode *node = NULL;
        ListNode *temp_node = NULL;
        uint16_t packet_id = 0;
        MessageTypes msg_type;

        if (NULL == (iter = list_iterator_new(pClient->list_sub_wait_ack, LIST_TAIL))) {
            LOG_ERROR("new list failed");
            HAL_MutexUnlock(pClient->lock_list_sub);
            return SUCCESS_RET;
        }

        for (;;) {
            node = list_iterator_next(iter);

            if (NULL != temp_node) {
                list_remove(pClient->list_sub_wait_ack, temp_node);
                temp_node = NULL;
            }

            if (NULL == node) {
                break; /* end of list */
            }

            UIoTSubInfo *sub_info = (UIoTSubInfo *) node->val;
            if (NULL == sub_info) {
                LOG_ERROR("node's value is invalid!");
                temp_node = node;
                continue;
            }

            /* remove invalid node */
            if (MQTT_NODE_STATE_INVALID == sub_info->node_state) {
                temp_node = node;
                continue;
            }

            if (pClient->is_connected <= 0) {
                continue;
            }

            /* check the request if timeout or not */
            if (left_ms(&sub_info->sub_start_time) > 0) {
                continue;
            }

            /* When arrive here, it means timeout to wait ACK */
            packet_id = sub_info->msg_id;
            msg_type = sub_info->type;

            /* Wait MQTT SUBSCRIBE ACK timeout */
            if (NULL != pClient->event_handler.h_fp) {
                MQTTEventMsg msg;

                if (SUBSCRIBE == msg_type) {
                    /* subscribe timeout */
                    msg.event_type = MQTT_EVENT_SUBSCRIBE_TIMEOUT;
                    msg.msg = (void *)(uintptr_t)packet_id;
                } else { 
                    /* unsubscribe timeout */
                    msg.event_type = MQTT_EVENT_UNSUBSCRIBE_TIMEOUT;
                    msg.msg = (void *)(uintptr_t)packet_id;
                }

                pClient->event_handler.h_fp(pClient, pClient->event_handler.context, &msg);
            }

            if (NULL != sub_info->handler.topic_filter)
                HAL_Free((void *)(sub_info->handler.topic_filter));
                
            temp_node = node;
        }

        list_iterator_destroy(iter);

    } while (0);

    HAL_MutexUnlock(pClient->lock_list_sub);

    return ret;
}

#ifdef __cplusplus
}
#endif
