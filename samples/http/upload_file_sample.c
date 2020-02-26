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

//upload file name saved in ufile
#define FILE_NAME       "upload_file.txt"
//data saved in file that you upload
#define UPLOAD_BUFFER   "This is a test file for upload test!"

static int running_state = 0;

static void http_upload_file_test_thread(void) 
{
    char md5[100];    
    char *authorization = (char *)HAL_Malloc(1024);
    memset(authorization, 0, 1024);
    char *put_url = (char *)HAL_Malloc(1024);
    memset(put_url, 0, 1024);
    int ret = SUCCESS_RET;
    
    http_client_buffer_md5(UPLOAD_BUFFER, sizeof(UPLOAD_BUFFER), md5);
    HAL_Printf("MD5:%s\n", md5);

    ret = IOT_GET_URL_AND_AUTH(PKG_USING_UCLOUD_IOT_SDK_PRODUCT_SN, PKG_USING_UCLOUD_IOT_SDK_DEVICE_SN, PKG_USING_UCLOUD_IOT_SDK_DEVICE_SECRET, \
                                FILE_NAME, UPLOAD_BUFFER, sizeof(UPLOAD_BUFFER), md5, authorization, put_url);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("get url and auth fail,ret:%d\r\n", ret);
        return;
    }

    HAL_Printf("get authorization:%s\n", authorization);
    HAL_Printf("get put_url:%s\n", put_url);

    //上传文件超时时间与文件长度相关
    ret = IOT_HTTP_UPLOAD_FILE(FILE_NAME, UPLOAD_BUFFER, sizeof(UPLOAD_BUFFER), md5, authorization, put_url, 10000);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("upload file fail,ret:%d\r\n", ret);
        return;
    }
    HAL_Printf("upload success\n");
    HAL_Free(authorization);
    HAL_Free(put_url);
    return;
}

static int http_upload_file_test_example(int argc, char **argv)
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

    tid = rt_thread_create("http_upload_file_test", (void (*)(void *))http_upload_file_test_thread, 
                            NULL, stack_size, RT_THREAD_PRIORITY_MAX / 2 - 1, 100);  

    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }

    return 0;
}


#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(http_upload_file_test_example, startup http upload_file example);
#endif

#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(http_upload_file_test_example, startup http upload_file example);
#endif


