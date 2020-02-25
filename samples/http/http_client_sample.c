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
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>

#include "uiot_import.h"
#include "ca.h"
#include "utils_httpc.h"
#include "uiot_export_http.h"

#define UIOT_PUBLISH_TOPIC            "%s/%s/upload/event"

static int running_state = 0;

static void http_publish_test_thread(void) 
{
    char *token = (char *)HAL_Malloc(1024);
    memset(token, 0, 1024);
    int ret = SUCCESS_RET;
    char *topic = (char *)HAL_Malloc(256);
    memset(topic, 0, 256);
    HAL_Snprintf((char *)topic, 256, UIOT_PUBLISH_TOPIC,PKG_USING_UCLOUD_IOT_SDK_PRODUCT_SN, PKG_USING_UCLOUD_IOT_SDK_DEVICE_SN);
    char *data = "{\"test\": \"18\"}";

    ret = IOT_HTTP_Get_Token(PKG_USING_UCLOUD_IOT_SDK_PRODUCT_SN, PKG_USING_UCLOUD_IOT_SDK_DEVICE_SN, PKG_USING_UCLOUD_IOT_SDK_DEVICE_SECRET, token);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("get Token fail,ret:%d\r\n", ret);
        return;
    }

    HAL_Printf("get token:%s\n", token);
    HAL_Printf("topic:%s\n", topic);
    ret = IOT_HTTP_Publish(token, topic, data, 5000);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Publish fail,ret:%d\r\n", ret);
        return;
    }
    HAL_Printf("Publish success\n");
    HAL_Free(token);
    HAL_Free(topic);
    return;
}

static int http_publish_test_example(int argc, char **argv)
{
    rt_thread_t tid;
    int stack_size = 8192;

    if (2 == argc)
    {
        if (!strcmp("start", argv[1]))
        {
            if (1 == running_state)
            {
                HAL_Printf("http_publish_test_example is already running\n");
                return 0;
            }            
        }
        else if (!strcmp("stop", argv[1]))
        {
            if (0 == running_state)
            {
                HAL_Printf("http_publish_test_example is already stopped\n");
                return 0;
            }
            running_state = 0;
            return 0;
        }
        else
        {
            HAL_Printf("Usage: http_publish_test_example start/stop");
            return 0;              
        }
    }
    else
    {
        HAL_Printf("Para err, usage: http_publish_test_example start/stop");
        return 0;
    }

    tid = rt_thread_create("http_publish_test", (void (*)(void *))http_publish_test_thread, 
                            NULL, stack_size, RT_THREAD_PRIORITY_MAX / 2 - 1, 100);  

    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }

    return 0;
}


#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(http_publish_test_example, startup http publish example);
#endif

#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(http_publish_test_example, startup http publish example);
#endif

