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

#include "uiot_defs.h"
#include "uiot_export_mqtt.h"

#include "dm_config.h"
#include "dm_internal.h"
#include "lite-utils.h"


DM_MQTT_CB_t g_dm_mqtt_cb[] = {
        {PROPERTY_RESTORE,        PROPERTY_RESTORE_TOPIC_TEMPLATE,        PROPERTY_RESTORE_REPLY_TOPIC_TEMPLATE,        dm_mqtt_property_restore_cb},
        {PROPERTY_POST,           PROPERTY_POST_TOPIC_TEMPLATE,           PROPERTY_POST_REPLY_TOPIC_TEMPLATE,           dm_mqtt_property_post_cb},
        {PROPERTY_SET,            PROPERTY_SET_REPLY_TOPIC_TEMPLATE,      PROPERTY_SET_TOPIC_TEMPLATE,                  dm_mqtt_property_set_cb},
        {PROPERTY_DESIRED_GET,    PROPERTY_DESIRED_GET_TOPIC_TEMPLATE,    PROPERTY_DESIRED_GET_REPLY_TOPIC_TEMPLATE,    dm_mqtt_property_desired_get_cb},
        {PROPERTY_DESIRED_DELETE, PROPERTY_DESIRED_DELETE_TOPIC_TEMPLATE, PROPERTY_DESIRED_DELETE_REPLY_TOPIC_TEMPLATE, dm_mqtt_property_desired_delete_cb},
        {EVENT_POST,              EVENT_POST_TOPIC_TEMPLATE,              EVENT_POST_REPLY_TOPIC_TEMPLATE,              dm_mqtt_event_post_cb},
        {COMMAND,                 COMMAND_REPLY_TOPIC_TEMPLATE,           COMMAND_TOPIC_TEMPLATE,                       dm_mqtt_command_cb}
};

static int _dm_mqtt_gen_topic_name(char *buf, size_t buf_len, const char *topic_template, const char *product_sn,
                                   const char *device_sn) {
    FUNC_ENTRY;

    int ret;
    ret = HAL_Snprintf(buf, buf_len, topic_template, product_sn, device_sn);

    if (ret < 0 || ret >= buf_len) {
        LOG_ERROR("HAL_Snprintf failed");
        FUNC_EXIT_RC(FAILURE_RET);
    }

    FUNC_EXIT_RC(SUCCESS_RET);
}

static int _dm_mqtt_gen_property_payload(char *buf, size_t buf_len, DM_Type type, int request_id, const char *payload) {
    FUNC_ENTRY;

    int ret;
    switch (type) {
        case PROPERTY_RESTORE:
            ret = HAL_Snprintf(buf, buf_len, "{\"RequestID\": \"%d\"}", request_id);
            break;
        case PROPERTY_POST:
            ret = HAL_Snprintf(buf, buf_len, "{\"RequestID\": \"%d\", \"Property\": %s}", request_id, payload);
            break;
        case PROPERTY_DESIRED_GET:
            ret = HAL_Snprintf(buf, buf_len, "{\"RequestID\": \"%d\", \"Require\": %s}", request_id, payload);
            break;
        case PROPERTY_DESIRED_DELETE:
            ret = HAL_Snprintf(buf, buf_len, "{\"RequestID\": \"%d\", \"Delete\": %s}", request_id, payload);
            break;
        default: FUNC_EXIT_RC(FAILURE_RET);
    }
    if (ret < 0 || ret >= buf_len) {
        LOG_ERROR("generate property payload failed");
        FUNC_EXIT_RC(FAILURE_RET);
    }

    FUNC_EXIT_RC(SUCCESS_RET);
}

static int _dm_mqtt_gen_event_payload(char *buf, size_t buf_len, int request_id, const char *identifier, const char *payload) {
    FUNC_ENTRY;

    int ret;
    ret = HAL_Snprintf(buf, buf_len, "{\"RequestID\": \"%d\", \"Identifier\": \"%s\", \"Output\": %s}", request_id,
                       identifier, payload);

    if (ret < 0 || ret >= buf_len) {
        LOG_ERROR("generate event payload failed");
        FUNC_EXIT_RC(FAILURE_RET);
    }

    FUNC_EXIT_RC(SUCCESS_RET);
}

static int _dm_mqtt_publish(DM_MQTT_Struct_t *handle, char *topic_name, int qos, const char *msg) {
    FUNC_ENTRY;

    int ret;
    PublishParams pub_params = DEFAULT_PUB_PARAMS;

    //暂不支持QOS2
    if (0 == qos) {
        pub_params.qos = QOS0;
    } else {
        pub_params.qos = QOS1;
    }
    pub_params.payload = (void *) msg;
    pub_params.payload_len = strlen(msg);

    ret = IOT_MQTT_Publish(handle->mqtt, topic_name, &pub_params);
    if (ret < 0) {
        LOG_ERROR("publish to topic: %s failed", topic_name);
        FUNC_EXIT_RC(FAILURE_RET);
    }

    FUNC_EXIT_RC(ret);
}

static void _dm_mqtt_common_reply_cb(MQTTMessage *message, CommonReplyCB cb) {
    FUNC_ENTRY;

    int8_t ret_code;
    char *request_id = NULL;
    char *ret_code_char = NULL;
    char *msg = NULL;

    if (NULL == (msg = HAL_Malloc(message->payload_len + 1))) {
        LOG_ERROR("allocate for message failed");
        FUNC_EXIT;
    }

    HAL_Snprintf(msg, message->payload_len + 1, "%s", message->payload);

    if (NULL == (ret_code_char = LITE_json_value_of((char *) "RetCode", msg))) {
        LOG_ERROR("allocate for ret_code_char failed");
        goto do_exit;
    }
    if (SUCCESS_RET != LITE_get_int8(&ret_code, ret_code_char)) {
        LOG_ERROR("get ret_code failed");
        goto do_exit;
    }
    if (NULL == (request_id = LITE_json_value_of((char *) "RequestID", msg))) {
        LOG_ERROR("allocate for request_id failed");
        goto do_exit;
    }

    cb(request_id, ret_code);

do_exit:
    HAL_Free(msg);
    HAL_Free(request_id);
    HAL_Free(ret_code_char);

    FUNC_EXIT;
}

void dm_mqtt_property_restore_cb(void *pClient, MQTTMessage *message, void *pContext) {
    FUNC_ENTRY;

    LOG_DEBUG("topic=%s", message->topic);
    LOG_INFO("len=%u, topic_msg=%.*s", message->payload_len, message->payload_len, (char *)message->payload);

    DM_MQTT_Struct_t *handle = (DM_MQTT_Struct_t *) pContext;
    if (NULL == handle->callbacks[PROPERTY_RESTORE]) {
        FUNC_EXIT;
    }

    PropertyRestoreCB cb;
    int8_t ret_code;
    char *request_id = NULL;
    char *ret_code_char = NULL;
    char *property = NULL;
    char *msg = NULL;

    if (NULL == (msg = HAL_Malloc(message->payload_len + 1))) {
        LOG_ERROR("allocate for message failed");
        FUNC_EXIT;
    }

    HAL_Snprintf(msg, message->payload_len + 1, "%s", message->payload);

    if (NULL == (ret_code_char = LITE_json_value_of((char *) "RetCode", msg))) {
        LOG_ERROR("allocate for ret_code_char failed");
        goto do_exit;
    }
    if (SUCCESS_RET != LITE_get_int8(&ret_code, ret_code_char)) {
        LOG_ERROR("get for ret_code failed");
        goto do_exit;
    }
    if (NULL == (request_id = LITE_json_value_of((char *) "RequestID", msg))) {
        LOG_ERROR("allocate for request_id failed");
        goto do_exit;
    }
    if (NULL == (property = LITE_json_value_of((char *) "Property", msg))) {
        LOG_ERROR("allocate for property failed");
        goto do_exit;
    }

    cb = (PropertyRestoreCB) handle->callbacks[PROPERTY_RESTORE];
    cb(request_id, ret_code, property);

do_exit:
    HAL_Free(msg);
    HAL_Free(ret_code_char);
    HAL_Free(property);
    HAL_Free(request_id);

    FUNC_EXIT;
}

void dm_mqtt_property_post_cb(void *pClient, MQTTMessage *message, void *pContext) {
    FUNC_ENTRY;

    LOG_DEBUG("topic=%s", message->topic);
    LOG_INFO("len=%u, topic_msg=%.*s", message->payload_len, message->payload_len, (char *)message->payload);

    DM_MQTT_Struct_t *handle = (DM_MQTT_Struct_t *) pContext;
    if (NULL == handle->callbacks[PROPERTY_POST]) {
        FUNC_EXIT;
    }

    _dm_mqtt_common_reply_cb(message, (CommonReplyCB) handle->callbacks[PROPERTY_POST]);

    FUNC_EXIT;
}

void dm_mqtt_property_set_cb(void *pClient, MQTTMessage *message, void *pContext) {
    FUNC_ENTRY;

    LOG_DEBUG("topic=%s", message->topic);
    LOG_INFO("len=%u, topic_msg=%.*s", message->payload_len, message->payload_len, (char *)message->payload);

    DM_MQTT_Struct_t *handle = (DM_MQTT_Struct_t *) pContext;
    if (NULL == handle->callbacks[PROPERTY_SET]) {
        FUNC_EXIT;
    }

    int ret;
    PropertySetCB cb;
    int cb_ret;
    char *request_id = NULL;
    char *property = NULL;
    char *msg_reply = NULL;
    char *topic = NULL;
    char *msg = NULL;

    if (NULL == (msg = HAL_Malloc(message->payload_len + 1))) {
        LOG_ERROR("allocate for message failed");
        FUNC_EXIT;
    }

    HAL_Snprintf(msg, message->payload_len + 1, "%s", message->payload);

    if (NULL == (request_id = LITE_json_value_of((char *) "RequestID", msg))) {
        LOG_ERROR("allocate for request_id failed");
        goto do_exit;
    }
    if (NULL == (property = LITE_json_value_of((char *) "Property", msg))) {
        LOG_ERROR("allocate for property failed");
        goto do_exit;
    }

    cb = (PropertySetCB) handle->callbacks[PROPERTY_SET];
    cb_ret = cb(request_id, property);

    if (NULL == (msg_reply = HAL_Malloc(DM_MSG_REPLY_BUF_LEN))) {
        LOG_ERROR("allocate for msg_reply failed");
        goto do_exit;
    }
    if (NULL == (topic = HAL_Malloc(DM_TOPIC_BUF_LEN))) {
        LOG_ERROR("allocate for topic failed");
        goto do_exit;
    }
    if (SUCCESS_RET != _dm_mqtt_gen_topic_name(topic, DM_TOPIC_BUF_LEN, handle->upstream_topic_templates[PROPERTY_SET],
            handle->product_sn, handle->device_sn)) {
        LOG_ERROR("generate topic name failed");
        goto do_exit;
    }
    ret = HAL_Snprintf(msg_reply, DM_MSG_REPLY_BUF_LEN, "{\"RequestID\": \"%s\", \"RetCode\": %d}", request_id, cb_ret);
    if (ret < 0 || ret >= DM_MSG_REPLY_BUF_LEN) {
        LOG_ERROR("HAL_Snprintf msg_reply failed");
        goto do_exit;
    }
    ret = _dm_mqtt_publish(handle, topic, 1, msg_reply);
    if (ret < 0) {
        LOG_ERROR("mqtt publish msg failed");
        goto do_exit;
    }

do_exit:
    HAL_Free(msg);
    HAL_Free(request_id);
    HAL_Free(property);
    HAL_Free(msg_reply);
    HAL_Free(topic);

    FUNC_EXIT;
}

void dm_mqtt_property_desired_get_cb(void *pClient, MQTTMessage *message, void *pContext) {
    FUNC_ENTRY;

    LOG_DEBUG("topic=%s", message->topic);
    LOG_INFO("len=%u, topic_msg=%.*s", message->payload_len, message->payload_len, (char *)message->payload);

    DM_MQTT_Struct_t *handle = (DM_MQTT_Struct_t *) pContext;
    if (NULL == handle->callbacks[PROPERTY_DESIRED_GET]) {
        FUNC_EXIT;
    }

    PropertyDesiredGetCB cb;
    int8_t ret_code;
    char *request_id = NULL;
    char *ret_code_char = NULL;
    char *desired = NULL;
    char *msg = NULL;

    if (NULL == (msg = HAL_Malloc(message->payload_len + 1))) {
        LOG_ERROR("allocate for message failed");
        goto do_exit;
    }

    HAL_Snprintf(msg, message->payload_len + 1, "%s", message->payload);

    if (NULL == (ret_code_char = LITE_json_value_of((char *) "RetCode", msg))) {
        LOG_ERROR("allocate for ret_code_char failed");
        goto do_exit;
    }
    if (SUCCESS_RET != LITE_get_int8(&ret_code, ret_code_char)) {
        LOG_ERROR("get ret_code failed");
        goto do_exit;
    }
    if (NULL == (request_id = LITE_json_value_of((char *) "RequestID", msg))) {
        LOG_ERROR("allocate for request_id failed");
        goto do_exit;
    }
    if (NULL == (desired = LITE_json_value_of((char *) "Desired", msg))) {
        LOG_ERROR("allocate for desired failed");
        goto do_exit;
    }

    cb = (PropertyDesiredGetCB) handle->callbacks[PROPERTY_DESIRED_GET];
    cb(request_id, ret_code, desired);

do_exit:
    HAL_Free(msg);
    HAL_Free(ret_code_char);
    HAL_Free(request_id);
    HAL_Free(desired);

    FUNC_EXIT;
}

void dm_mqtt_property_desired_delete_cb(void *pClient, MQTTMessage *message, void *pContext) {
    FUNC_ENTRY;

    LOG_DEBUG("topic=%s", message->topic);
    LOG_INFO("len=%u, topic_msg=%.*s", message->payload_len, message->payload_len, (char *)message->payload);

    DM_MQTT_Struct_t *handle = (DM_MQTT_Struct_t *) pContext;
    if (NULL == handle->callbacks[PROPERTY_DESIRED_DELETE]) {
        FUNC_EXIT;
    }

    _dm_mqtt_common_reply_cb(message, (CommonReplyCB) handle->callbacks[PROPERTY_DESIRED_DELETE]);

    FUNC_EXIT;
}

void dm_mqtt_event_post_cb(void *pClient, MQTTMessage *message, void *pContext) {
    FUNC_ENTRY;

    LOG_DEBUG("topic=%s", message->topic);
    LOG_INFO("len=%u, topic_msg=%.*s", strlen(message->payload), message->payload_len, (char *)message->payload);

    DM_MQTT_Struct_t *handle = (DM_MQTT_Struct_t *) pContext;
    if (NULL == handle->callbacks[EVENT_POST]) {
        FUNC_EXIT;
    }

    _dm_mqtt_common_reply_cb(message, (CommonReplyCB) handle->callbacks[EVENT_POST]);

    FUNC_EXIT;
}

void dm_mqtt_command_cb(void *pClient, MQTTMessage *message, void *pContext) {
    FUNC_ENTRY;

    LOG_DEBUG("topic=%s", message->topic);
    LOG_INFO("len=%u, topic_msg=%.*s", message->payload_len, message->payload_len, (char *)message->payload);

    DM_MQTT_Struct_t *handle = (DM_MQTT_Struct_t *) pContext;
    if (NULL == handle->callbacks[COMMAND]) {
        FUNC_EXIT;
    }

    CommandCB cb;
    int cb_ret;
    char *request_id = NULL;
    char *identifier = NULL;
    char *input = NULL;
    char *output = NULL;
    char *topic = NULL;
    char *cmd_reply = NULL;
    char *msg = NULL;

    if (NULL == (msg = HAL_Malloc(message->payload_len + 1))) {
        LOG_ERROR("allocate for message failed");
        FUNC_EXIT;
    }

    HAL_Snprintf(msg, message->payload_len + 1, "%s", message->payload);

    if (NULL == (request_id = LITE_json_value_of((char *) "RequestID", msg))) {
        LOG_ERROR("allocate for request_id failed");
        goto do_exit;
    }
    if (NULL == (identifier = LITE_json_value_of((char *) "Identifier", msg))) {
        LOG_ERROR("allocate for identifier failed");
        goto do_exit;
    }

    if (NULL == (input = LITE_json_value_of((char *) "Input", msg))) {
        LOG_ERROR("allocate for input failed");
        goto do_exit;
    }

    cb = (CommandCB) handle->callbacks[COMMAND];
    cb_ret = cb(request_id, identifier, input, &output);

    if (NULL == output) {
        LOG_ERROR("output error");
        goto do_exit;
    }
    if (NULL == (cmd_reply = HAL_Malloc(DM_CMD_REPLY_BUF_LEN))) {
        LOG_ERROR("allocate for cmd_reply failed");
        goto do_exit;
    }
    if (NULL == (topic = HAL_Malloc(DM_TOPIC_BUF_LEN))) {
        LOG_ERROR("allocate for topic failed");
        goto do_exit;
    }
    int ret = HAL_Snprintf(topic, DM_TOPIC_BUF_LEN, handle->upstream_topic_templates[COMMAND], handle->product_sn,
                           handle->device_sn, request_id);
    if (ret < 0 || ret > DM_TOPIC_BUF_LEN) {
        LOG_ERROR("topic error");
        goto do_exit;
    }

    ret = HAL_Snprintf(cmd_reply, DM_CMD_REPLY_BUF_LEN,
                       "{\"RequestID\": \"%s\", \"RetCode\": %d, \"Identifier\": \"%s\", \"Output\": %s}",
                       request_id, cb_ret, identifier, output);
    if (ret < 0 || ret > DM_CMD_REPLY_BUF_LEN) {
        LOG_ERROR("generate cmd_reply msg failed");
        goto do_exit;
    }
    ret = _dm_mqtt_publish(handle, topic, 1, cmd_reply);
    if (ret < 0) {
        LOG_ERROR("mqtt publish msg failed");
        goto do_exit;
    }

do_exit:
    HAL_Free(msg);
    HAL_Free(request_id);
    HAL_Free(identifier);
    HAL_Free(input);
    HAL_Free(output);
    HAL_Free(topic);
    HAL_Free(cmd_reply);

    FUNC_EXIT;
}

int _dsc_mqtt_register_callback(DM_MQTT_Struct_t *handle, DM_Type dm_type, void *callback) {
    FUNC_ENTRY;

    int ret;
    if (NULL == handle || callback == NULL) {
        LOG_ERROR("params error!");
        FUNC_EXIT_RC(FAILURE_RET);
    }
    handle->callbacks[dm_type] = callback;
    handle->upstream_topic_templates[dm_type] = g_dm_mqtt_cb[dm_type].upstream_topic_template;
    handle->downstream_topic_templates[dm_type] = g_dm_mqtt_cb[dm_type].downstream_topic_template;

    char topic[DM_TOPIC_BUF_LEN];
    ret = _dm_mqtt_gen_topic_name(topic, DM_TOPIC_BUF_LEN, handle->downstream_topic_templates[dm_type],
                                  handle->product_sn, handle->device_sn);
    if (ret < 0) {
        LOG_ERROR("generate topic name failed");
        FUNC_EXIT_RC(FAILURE_RET);
    }

    SubscribeParams sub_params = DEFAULT_SUB_PARAMS;
    sub_params.on_message_handler = g_dm_mqtt_cb[dm_type].callback;
    sub_params.qos = QOS1;
    sub_params.user_data = handle;

    ret = IOT_MQTT_Subscribe(handle->mqtt, topic, &sub_params);
    if (ret < 0) {
        LOG_ERROR("mqtt subscribe failed!");
        FUNC_EXIT_RC(FAILURE_RET);
    }

    FUNC_EXIT_RC(SUCCESS_RET);
}


DEFINE_DM_CALLBACK(PROPERTY_RESTORE, PropertyRestoreCB)

DEFINE_DM_CALLBACK(PROPERTY_POST, CommonReplyCB)

DEFINE_DM_CALLBACK(PROPERTY_SET, PropertySetCB)

DEFINE_DM_CALLBACK(PROPERTY_DESIRED_GET, PropertyDesiredGetCB)

DEFINE_DM_CALLBACK(PROPERTY_DESIRED_DELETE, CommonReplyCB)

DEFINE_DM_CALLBACK(EVENT_POST, CommonReplyCB)

DEFINE_DM_CALLBACK(COMMAND, CommandCB)

void *dsc_init(const char *product_sn, const char *device_sn, void *channel, void *context) {
    FUNC_ENTRY;

    DM_MQTT_Struct_t *h_dsc = NULL;

    if (NULL == (h_dsc = HAL_Malloc(sizeof(DM_MQTT_Struct_t)))) {
        LOG_ERROR("allocate for h_dsc failed");
        FUNC_EXIT_RC(NULL);
    }

    memset(h_dsc, 0, sizeof(DM_MQTT_Struct_t));

    h_dsc->mqtt = channel;
    h_dsc->product_sn = product_sn;
    h_dsc->device_sn = device_sn;
    h_dsc->context = context;

    FUNC_EXIT_RC(h_dsc);
}

int dsc_deinit(void *handle) {
    FUNC_ENTRY;

    if (NULL != handle) {
        HAL_Free(handle);
    }

    FUNC_EXIT_RC(SUCCESS_RET);
}

int dm_mqtt_property_report_publish(DM_MQTT_Struct_t *handle, DM_Type type, int request_id, const char *payload) {
    FUNC_ENTRY;

    int ret = FAILURE_RET;
    char *msg_report = NULL;
    char *topic = NULL;

    if (NULL == (msg_report = HAL_Malloc(DM_MSG_REPORT_BUF_LEN))) {
        LOG_ERROR("allocate for msg_report failed");
        goto do_exit;
    }
    if (NULL == (topic = HAL_Malloc(DM_TOPIC_BUF_LEN))) {
        LOG_ERROR("allocate for topic failed");
        goto do_exit;
    }

    if (SUCCESS_RET != _dm_mqtt_gen_topic_name(topic, DM_TOPIC_BUF_LEN, handle->upstream_topic_templates[type],
            handle->product_sn, handle->device_sn)) {
        LOG_ERROR("generate topic failed");
        goto do_exit;
    }

    ret = _dm_mqtt_gen_property_payload(msg_report, DM_MSG_REPORT_BUF_LEN, type, request_id, payload);
    if (ret < 0) {
        LOG_ERROR("generate msg_report failed");
        ret = FAILURE_RET;
        goto do_exit;
    }
    ret = _dm_mqtt_publish(handle, topic, 1, msg_report);
    if (ret < 0) {
        LOG_ERROR("mqtt publish msg failed");
    }

do_exit:
    HAL_Free(topic);
    HAL_Free(msg_report);

    FUNC_EXIT_RC(ret);
}

int dm_mqtt_event_publish(DM_MQTT_Struct_t *handle, int request_id, const char *identifier, const char *payload) {
    FUNC_ENTRY;

    int ret = FAILURE_RET;
    char *msg_report = NULL;
    char *topic = NULL;

    if (NULL == (msg_report = HAL_Malloc(DM_EVENT_POST_BUF_LEN))) {
        LOG_ERROR("allocate for msg_report failed");
        goto do_exit;
    }
    if (NULL == (topic = HAL_Malloc(DM_TOPIC_BUF_LEN))) {
        LOG_ERROR("allocate for topic failed");
        goto do_exit;
    }

    if (SUCCESS_RET != _dm_mqtt_gen_topic_name(topic, DM_TOPIC_BUF_LEN, handle->upstream_topic_templates[EVENT_POST],
            handle->product_sn, handle->device_sn)) {
        LOG_ERROR("generate topic failed");
        goto do_exit;
    }

    ret = _dm_mqtt_gen_event_payload(msg_report, DM_EVENT_POST_BUF_LEN, request_id, identifier, payload);
    if (ret < 0) {
        LOG_ERROR("generate msg_report failed");
        goto do_exit;
    }
    ret = _dm_mqtt_publish(handle, topic, 1, msg_report);
    if (ret < 0) {
        LOG_ERROR("mqtt publish msg failed");
    }

do_exit:
    HAL_Free(topic);
    HAL_Free(msg_report);

    FUNC_EXIT_RC(ret);
}