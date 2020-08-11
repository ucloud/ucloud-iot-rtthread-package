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
#include "uiot_internal.h"

#include "dm_internal.h"

void *IOT_DM_Init(const char *product_sn, const char *device_sn, void *ch_signal)
{
    POINTER_VALID_CHECK(product_sn, NULL);
    POINTER_VALID_CHECK(device_sn, NULL);
    POINTER_VALID_CHECK(ch_signal, NULL);

    DM_Struct_t *h_dm = NULL;

    if (NULL == (h_dm = HAL_Malloc(sizeof(DM_Struct_t)))) {
        LOG_ERROR("allocate failed");
        return NULL;
    }
    memset(h_dm, 0, sizeof(DM_Struct_t));

    h_dm->ch_signal = dsc_init(product_sn, device_sn, ch_signal, h_dm);
    if (NULL == h_dm->ch_signal) {
        LOG_ERROR("initialize signal channel failed");
        HAL_Free(h_dm);
        return NULL;
    }
    return h_dm;
}

int IOT_DM_Destroy(void *handle)
{
    POINTER_VALID_CHECK(handle, FAILURE_RET);

    DM_Struct_t *h_dm = (DM_Struct_t*) handle;

    dsc_deinit(h_dm->ch_signal);
    HAL_Free(h_dm);
    return SUCCESS_RET;
}

int IOT_DM_Property_Report(void *handle, DM_Type type, int request_id, const char *payload)
{
    POINTER_VALID_CHECK(handle, FAILURE_RET);

    DM_Struct_t *h_dm = (DM_Struct_t*) handle;

    return dm_mqtt_property_report_publish(h_dm->ch_signal, type, request_id, payload);
}

int IOT_DM_Property_ReportEx(void *handle, DM_Type type, int request_id, int property_num, ...)
{
    POINTER_VALID_CHECK(handle, FAILURE_RET);
    int loop = 0;
    int ret = 0;
    DM_Struct_t *h_dm = (DM_Struct_t*) handle;

    va_list pArgs;
    va_start(pArgs, property_num);

    DM_Property_t *property = (DM_Property_t *)HAL_Malloc(property_num * sizeof(DM_Property_t));

    for(loop = 0; loop < property_num; loop++)
    {
        DM_Property_t *property_node;
        property_node  = va_arg(pArgs, DM_Property_t *);
        property[loop] = *property_node;
    }
    
    va_end(pArgs);
    
    ret = dm_mqtt_property_report_publish_Ex(h_dm->ch_signal, type, request_id, property, property_num);
    HAL_Free(property);
    return ret;
}

int IOT_DM_TriggerEvent(void *handle, int request_id, const char *identifier, const char *payload)
{
    POINTER_VALID_CHECK(handle, FAILURE_RET);

    DM_Struct_t *h_dm = (DM_Struct_t*) handle;

    return dm_mqtt_event_publish(h_dm->ch_signal, request_id, identifier, payload);
}

int IOT_DM_TriggerEventEx(void *handle, int request_id, DM_Event_t *event)
{
    POINTER_VALID_CHECK(handle, FAILURE_RET);

    DM_Struct_t *h_dm = (DM_Struct_t*) handle;

    return dm_mqtt_event_publish_Ex(h_dm->ch_signal, request_id, event);
}

int IOT_DM_GenCommandOutput(char *output, int property_num, ...)
{
    POINTER_VALID_CHECK(output, FAILURE_RET);
    int ret = 0;
    int loop = 0;

    va_list pArgs;
    va_start(pArgs, property_num);

    DM_Property_t *property = (DM_Property_t *)HAL_Malloc(property_num * sizeof(DM_Property_t));

    for(loop = 0; loop < property_num; loop++)
    {
        DM_Property_t *property_node;
        property_node  = va_arg(pArgs, DM_Property_t *);
        property[loop] = *property_node;
    }
    
    va_end(pArgs);
    
    ret = dm_gen_properties_payload(property, property_num, PROPERTY_POST, false, output);    
    HAL_Free(property);
    return ret;
}

int IOT_DM_Yield(void *handle, uint32_t timeout_ms)
{
    POINTER_VALID_CHECK(handle, FAILURE_RET);

    DM_Struct_t *h_dm = (DM_Struct_t*) handle;

    return IOT_MQTT_Yield(((DM_MQTT_Struct_t *)h_dm->ch_signal)->mqtt, timeout_ms);
}


