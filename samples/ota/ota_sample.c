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

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "uiot_export.h"
#include "uiot_import.h"
#include "uiot_export_ota.h"

#define OTA_BUF_LEN (1024)
static int running_state = 0;

static void event_handler(void *pClient, void *handle_context, MQTTEventMsg *msg)
{
    switch(msg->event_type) {
        case MQTT_EVENT_UNDEF:
            LOG_INFO("undefined event occur.\n");
            break;

        case MQTT_EVENT_DISCONNECT:
            LOG_INFO("MQTT disconnect.\n");
            break;

        case MQTT_EVENT_RECONNECT:
            LOG_INFO("MQTT reconnect.\n");
            break;

        case MQTT_EVENT_SUBSCRIBE_SUCCESS:
            LOG_INFO("subscribe success.\n");
            break;

        case MQTT_EVENT_SUBSCRIBE_TIMEOUT:
            LOG_INFO("subscribe wait ack timeout.\n");
            break;

        case MQTT_EVENT_SUBSCRIBE_NACK:
            LOG_INFO("subscribe nack.\n");
            break;

        case MQTT_EVENT_PUBLISH_SUCCESS:
            LOG_INFO("publish success.\n");
            break;

        case MQTT_EVENT_PUBLISH_TIMEOUT:
            LOG_INFO("publish timeout.\n");
            break;

        case MQTT_EVENT_PUBLISH_NACK:
            LOG_INFO("publish nack.\n");
            break;
        default:
            LOG_INFO("Should NOT arrive here.\n");
            break;
    }
}


static int _setup_connect_init_params(MQTTInitParams* initParams)
{
    initParams->device_sn = PKG_USING_UCLOUD_IOT_SDK_DEVICE_SN;
    initParams->product_sn = PKG_USING_UCLOUD_IOT_SDK_PRODUCT_SN;
    initParams->device_secret = PKG_USING_UCLOUD_IOT_SDK_DEVICE_SECRET;

    initParams->command_timeout = UIOT_MQTT_COMMAND_TIMEOUT;    
    initParams->keep_alive_interval = UIOT_MQTT_KEEP_ALIVE_INTERNAL;
    initParams->auto_connect_enable = 1;
    initParams->event_handler.h_fp = event_handler;

    return SUCCESS_RET;
}

static void ota_test_thread(void)
{
    int rc;

    MQTTInitParams init_params = DEFAULT_MQTT_INIT_PARAMS;
    rc = _setup_connect_init_params(&init_params);
    if (rc != SUCCESS_RET) {
        return;
    }

    void *client = IOT_MQTT_Construct(&init_params);
    if (client != NULL) {
        LOG_INFO("MQTT Construct Success");
    } else {
        LOG_ERROR("MQTT Construct Failed");
        return;
    }

    void *h_ota = IOT_OTA_Init(PKG_USING_UCLOUD_IOT_SDK_PRODUCT_SN, PKG_USING_UCLOUD_IOT_SDK_DEVICE_SN, client);
    if (NULL == h_ota) {
        IOT_MQTT_Destroy(&client);
        LOG_ERROR("init OTA failed");
        return;
    }

    /* Must report version first */
    if (IOT_OTA_ReportVersion(h_ota, "1.0.0") < 0) {
        LOG_ERROR("report OTA version failed");
        return;
    }

    if (IOT_OTA_RequestFirmware(h_ota, "1.0.0") < 0) {
        LOG_ERROR("Request firmware failed");
        return;
    }

    do {
        IOT_MQTT_Yield(client, 10);
    } while(1);

}

static int ota_test_example(int argc, char **argv)
{
    rt_thread_t tid;
    int stack_size = 10240;

    if (2 == argc)
    {
        if (!strcmp("start", argv[1]))
        {
            if (1 == running_state)
            {
                HAL_Printf("mqtt_ota_example is already running\n");
                return 0;
            }            
        }
        else if (!strcmp("stop", argv[1]))
        {
            if (0 == running_state)
            {
                HAL_Printf("mqtt_ota_example is already stopped\n");
                return 0;
            }
            running_state = 0;
            return 0;
        }
        else
        {
            HAL_Printf("Usage: mqtt_ota_example start/stop");
            return 0;              
        }
    }
    else
    {
        HAL_Printf("Para err, usage: mqtt_ota_example start/stop");
        return 0;
    }
    
    tid = rt_thread_create("ota_test", (void (*)(void *))ota_test_thread, 
                            NULL, stack_size, RT_THREAD_PRIORITY_MAX / 2 - 1, 100);  

    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }

    return 0;
}


#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(ota_test_example, startup mqtt ota example);
#endif

#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(ota_test_example, startup mqtt ota example);
#endif

