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

#include <string.h>

#include "uiot_defs.h"
#include "uiot_export_mqtt.h"
#include "uiot_export_ota.h"
#include "uiot_import.h"

#include "ota_config.h"
#include "ota_internal.h"

static int _ota_mqtt_gen_topic_name(char *buf, size_t buf_len, const char *ota_topic_type, const char *product_sn,
                                    const char *device_sn)
{
    FUNC_ENTRY;

    int ret;
    ret = HAL_Snprintf(buf, buf_len, OTA_TOPIC_TEMPLATE, product_sn, device_sn, ota_topic_type);

    if(ret >= buf_len) {
        FUNC_EXIT_RC(ERR_OTA_GENERAL_FAILURE);
    }

    if (ret < 0) {
        LOG_ERROR("HAL_Snprintf failed");
        FUNC_EXIT_RC(ERR_OTA_GENERAL_FAILURE);
    }

    FUNC_EXIT_RC(SUCCESS_RET);
}

static int _ota_mqtt_publish(OTA_MQTT_Struct_t *handle, const char *topic_type, int qos, const char *msg)
{
    FUNC_ENTRY;

    int ret;
    char topic_name[OTA_TOPIC_BUF_LEN];
    PublishParams pub_params = DEFAULT_PUB_PARAMS;

    //暂不支持QOS2
    if (0 == qos) {
        pub_params.qos = QOS0;
    } else {
        pub_params.qos = QOS1;
    }
    pub_params.payload = (void *)msg;
    pub_params.payload_len = strlen(msg);

    ret = _ota_mqtt_gen_topic_name(topic_name, OTA_TOPIC_BUF_LEN, topic_type, handle->product_sn, handle->device_sn);
    if (ret < 0) {
       LOG_ERROR("generate topic name of info failed");
       FUNC_EXIT_RC(ERR_OTA_GENERAL_FAILURE);
    }

    ret = IOT_MQTT_Publish(handle->mqtt, topic_name, &pub_params);
    if (ret < 0) {
        LOG_ERROR("publish to topic: %s failed", topic_name);
        FUNC_EXIT_RC(ERR_OTA_OSC_FAILED);
    }

    FUNC_EXIT_RC(ret);
}

static void _ota_mqtt_upgrade_cb(void *pClient, MQTTMessage *message, void *pContext)
{
    FUNC_ENTRY;

    OTA_MQTT_Struct_t *handle = (OTA_MQTT_Struct_t *) pContext;

    LOG_DEBUG("topic=%s", message->topic);
    LOG_INFO("len=%u, topic_msg=%.*s", message->payload_len, message->payload_len, (char *)message->payload);

    if (NULL != handle->msg_callback) {
        handle->msg_callback(handle->context, message->payload, message->payload_len);
    }

    FUNC_EXIT;
}

void *osc_init(const char *product_sn, const char *device_sn, void *channel, OnOTAMessageCallback callback,
               void *context)
{
    int ret;
    OTA_MQTT_Struct_t *h_osc = NULL;

    if (NULL == (h_osc = HAL_Malloc(sizeof(OTA_MQTT_Struct_t)))) {
        LOG_ERROR("allocate for h_osc failed");
        goto do_exit;
    }

    memset(h_osc, 0, sizeof(OTA_MQTT_Struct_t));

    h_osc->mqtt = channel;
    h_osc->product_sn = product_sn;
    h_osc->device_sn = device_sn;
    h_osc->msg_callback = callback;
    h_osc->context = context;
    h_osc->msg_list = list_new();
    h_osc->msg_mutex = HAL_MutexCreate();
    if (h_osc->msg_mutex == NULL)
        goto do_exit;

    /* subscribe the OTA topic: "/$system/$(product_sn)/$(device_sn)/ota/downstream" */
    ret = _ota_mqtt_gen_topic_name(h_osc->topic_upgrade, OTA_TOPIC_BUF_LEN, OTA_DOWNSTREAM_TOPIC_TYPE, product_sn,
                                   device_sn);
    if (ret < 0) {
        LOG_ERROR("generate topic name of upgrade failed");
        goto do_exit;
    }

    SubscribeParams sub_params = DEFAULT_SUB_PARAMS;
    sub_params.on_message_handler = _ota_mqtt_upgrade_cb;
    sub_params.qos = QOS1;
    sub_params.user_data = h_osc;

    ret = IOT_MQTT_Subscribe(channel, h_osc->topic_upgrade, &sub_params);
    if (ret < 0) {
        LOG_ERROR("ota mqtt subscribe failed!");
        goto do_exit;
    }

    return h_osc;

do_exit:
    if (NULL != h_osc) {
         HAL_Free(h_osc);
    }

    return NULL;
}

int osc_deinit(void *handle)
{
    FUNC_ENTRY;

    OTA_MQTT_Struct_t  *h_osc = handle;

    list_destroy(h_osc->msg_list);
    HAL_MutexDestroy(h_osc->msg_mutex);

    if (NULL != handle) {
        HAL_Free(handle);
    }

    FUNC_EXIT_RC(SUCCESS_RET);
}

/* report progress of OTA */
int osc_report_progress(void *handle, const char *msg)
{
    return _ota_mqtt_publish(handle, OTA_UPSTREAM_TOPIC_TYPE, QOS0, msg);
}

/* report version of firmware */
int osc_upstream_publish(void *handle, const char *msg)
{
    return _ota_mqtt_publish(handle, OTA_UPSTREAM_TOPIC_TYPE, QOS1, msg);
}
