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

#ifndef C_SDK_DM_INTERNAL_H_
#define C_SDK_DM_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "uiot_export_dm.h"

typedef struct  {
    void       *ch_signal;
} DM_Struct_t;

typedef struct {
    int               dm_type;
    char              *upstream_topic_template;
    char              *downstream_topic_template;
    OnMessageHandler  callback;
} DM_MQTT_CB_t;

typedef struct {
    void       *mqtt;
    const char *product_sn;
    const char *device_sn;
    void       *callbacks[DM_TYPE_MAX];
    char       *upstream_topic_templates[DM_TYPE_MAX];
    char       *downstream_topic_templates[DM_TYPE_MAX];
    void       *context;
} DM_MQTT_Struct_t;

#define DEFINE_DM_CALLBACK(type, cb_type)  int uiot_register_for_##type(void *handle, cb_type cb) { \
        if (type < 0 || type >= sizeof(g_dm_mqtt_cb)/sizeof(DM_MQTT_CB_t)) {return -1;} \
        _dsc_mqtt_register_callback((DM_MQTT_Struct_t *)(((DM_Struct_t *)handle)->ch_signal), type, (void *)cb);return 0;}


void dm_mqtt_property_restore_cb(void *pClient, MQTTMessage *message, void *pContext);

void dm_mqtt_property_post_cb(void *pClient, MQTTMessage *message, void *pContext);

void dm_mqtt_property_set_cb(void *pClient, MQTTMessage *message, void *pContext);

void dm_mqtt_property_desired_get_cb(void *pClient, MQTTMessage *message, void *pContext);

void dm_mqtt_property_desired_delete_cb(void *pClient, MQTTMessage *message, void *pContext);

void dm_mqtt_event_post_cb(void *pClient, MQTTMessage *message, void *pContext);

void dm_mqtt_command_cb(void *pClient, MQTTMessage *message, void *pContext);


void *dsc_init(const char *product_sn, const char *device_sn, void *channel, void *context);

int dsc_deinit(void *handle);

int dm_gen_properties_payload(DM_Property_t *property, int property_num, DM_Type type, bool value_key, char *properties_payload);

int dm_mqtt_property_report_publish(DM_MQTT_Struct_t *handle, DM_Type type, int request_id, const char *payload);

int dm_mqtt_property_report_publish_Ex(DM_MQTT_Struct_t *handle, DM_Type type, int request_id, DM_Property_t *property, int property_num);

int dm_mqtt_event_publish(DM_MQTT_Struct_t *handle, int request_id, const char *identifier, const char *payload);

int dm_mqtt_event_publish_Ex(DM_MQTT_Struct_t *handle, int request_id, DM_Event_t *event);

#ifdef __cplusplus
}
#endif

#endif //C_SDK_DM_INTERNAL_H_
