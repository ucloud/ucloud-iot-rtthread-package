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
#include <string.h>

#include "utils_httpc.h"
#include "lite-utils.h"
#include "ca.h"
#include "utils_sha2.h"

int IOT_HTTP_Get_Token(const char *product_sn, const char *device_sn, const char *device_sercret, char *token)
{
    int ret = SUCCESS_RET;
    http_client_t *http_client_post = (http_client_t *)HAL_Malloc(sizeof(http_client_t));
    if(NULL == http_client_post)
    {
        LOG_ERROR("http_client_post malloc fail\n");
        return FAILURE_RET;
    }
    http_client_data_t *http_data_post = (http_client_data_t *)HAL_Malloc(sizeof(http_client_data_t));
    if(NULL == http_data_post)
    {
        HAL_Free(http_client_post);
        LOG_ERROR("http_data_post malloc fail\n");
        return FAILURE_RET;
    }
    
    memset(http_client_post, 0, sizeof(http_client_t));
    memset(http_data_post, 0, sizeof(http_client_data_t));

    http_data_post->response_buf = (char *)HAL_Malloc(1024);
    if(NULL == http_data_post->response_buf)
    {
        HAL_Free(http_client_post);
        HAL_Free(http_data_post);
        LOG_ERROR("http_data_post->response_buf malloc fail\n");
        return FAILURE_RET;
    }
    memset(http_data_post->response_buf, 0, 1024);
    http_data_post->response_buf_len = 1024;
    http_data_post->post_content_type = (char *)"application/json";
    http_data_post->post_buf = (unsigned char *)HAL_Malloc(1024);
    if(NULL == http_data_post->post_buf)
    {
        HAL_Free(http_client_post);
        HAL_Free(http_data_post->response_buf);
        HAL_Free(http_data_post);
        LOG_ERROR("http_data_post->post_buf malloc fail\n");
        return FAILURE_RET;
    }
    memset(http_data_post->post_buf, 0, 1024);
    
    HAL_Snprintf((char *)http_data_post->post_buf, 1024, "{\"ProductSN\":\"%s\",\"DeviceSN\":\"%s\"}",product_sn, device_sn);
    http_data_post->post_buf_len = strlen((char *)http_data_post->post_buf);
    uint8_t mac_output_hex[32] = {0};
    char mac_output_char[65] = {0};

    utils_hmac_sha256((const uint8_t *)http_data_post->post_buf, http_data_post->post_buf_len, (const uint8_t *)device_sercret, strlen(device_sercret), mac_output_hex); 
    LITE_hexbuf_convert((unsigned char *)mac_output_hex, mac_output_char, 32, 0);
    LOG_DEBUG("hmac:%s\r\n",mac_output_char);

    http_client_post->header = (char *)HAL_Malloc(1024);
    if(NULL == http_client_post->header)
    {
        HAL_Free(http_client_post);
        HAL_Free(http_data_post->response_buf);
        HAL_Free(http_data_post->post_buf);
        HAL_Free(http_data_post);
        LOG_ERROR("http_client_post->header malloc fail\n");
        return FAILURE_RET;
    }
    memset(http_client_post->header, 0, 1024);
    HAL_Snprintf(http_client_post->header, 1024, "Content-Type: application/json\r\nAuthorization: %s\r\nbody: %s\r\n",mac_output_char,http_data_post->post_buf);

    const char *ca_crt = iot_https_ca_get();
    char *url = (char *)"https://http-cn-sh2.iot.ucloud.cn/auth";

    ret = http_client_common(http_client_post, url, 443, ca_crt, HTTP_POST, http_data_post,5000);
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("HTTP_POST error\n");
        goto end;
    }
    
    ret = http_client_recv_data(http_client_post, 5000, http_data_post);
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("http_client_recv_data error\n");
        goto end;
    }
    LOG_DEBUG("response_buf:%s\n",http_data_post->response_buf);

    char *temp = LITE_json_value_of((char *)"Token", http_data_post->response_buf);
    if(NULL == temp)
    {
        LOG_ERROR("parse Token error\n");
        char *temp_message = LITE_json_value_of((char *)"Message", http_data_post->response_buf);
        LOG_ERROR("%s\n", temp_message);
        HAL_Free(temp_message);
        goto end;
    }
    strncpy(token,temp,strlen(temp));
    token[strlen(temp)+1] = '\0';
    LOG_DEBUG("token:%s\n",token);
    HAL_Free(temp);
end:    
    http_client_close(http_client_post);    
    HAL_Free(http_client_post->header);    
    HAL_Free(http_client_post);
    HAL_Free(http_data_post->response_buf);
    HAL_Free(http_data_post->post_buf);
    HAL_Free(http_data_post);  
    return ret;
}

int IOT_HTTP_Publish(char *token, char *topic, char *data, uint32_t timeout_ms)
{
    int ret = SUCCESS_RET;
    http_client_t *http_client_post = (http_client_t *)HAL_Malloc(sizeof(http_client_t));
    if(NULL == http_client_post)
    {
        LOG_ERROR("http_client_post malloc fail\n");
        return FAILURE_RET;
    }
    http_client_data_t *http_data_post = (http_client_data_t *)HAL_Malloc(sizeof(http_client_data_t));
    if(NULL == http_data_post)
    {
        HAL_Free(http_client_post);
        LOG_ERROR("http_data_post malloc fail\n");
        return FAILURE_RET;
    }
    
    memset(http_client_post, 0, sizeof(http_client_t));
    memset(http_data_post, 0, sizeof(http_client_data_t));

    http_data_post->response_buf = (char *)HAL_Malloc(1024);
    if(NULL == http_data_post->response_buf)
    {
        HAL_Free(http_client_post);
        HAL_Free(http_data_post);
        LOG_ERROR("http_data_post->response_buf malloc fail\n");
        return FAILURE_RET;
    }
    memset(http_data_post->response_buf, 0, 1024);
    http_data_post->response_buf_len = 1024;
    http_data_post->post_content_type = (char *)"application/octet-stream";
    http_data_post->post_buf = (unsigned char *)HAL_Malloc(1024);
    if(NULL == http_data_post->post_buf)
    {
        HAL_Free(http_client_post);        
        HAL_Free(http_data_post->response_buf);
        HAL_Free(http_data_post);
        LOG_ERROR("http_data_post->post_buf malloc fail\n");
        return FAILURE_RET;
    }
    memset(http_data_post->post_buf, 0, 1024);
    
    HAL_Snprintf((char *)http_data_post->post_buf, 1024, "%s",data);
    http_data_post->post_buf_len = strlen((char *)http_data_post->post_buf);
    
    http_client_post->header = (char *)HAL_Malloc(1024);
    if(NULL == http_client_post->header)
    {
        HAL_Free(http_client_post);        
        HAL_Free(http_data_post->response_buf);
        HAL_Free(http_data_post->post_buf);
        HAL_Free(http_data_post);
        LOG_ERROR("http_client_post->header malloc fail\n");
        return FAILURE_RET;
    }
    memset(http_client_post->header, 0, 1024);
    HAL_Snprintf(http_client_post->header, 1024, "Password: %s\r\nContent-Type: application/octet-stream\r\nbody: %s\r\n", token, http_data_post->post_buf);

    const char *ca_crt = iot_https_ca_get();
    char *url = (char *)HAL_Malloc(256);
    if(NULL == url)
    {
        goto end;
        LOG_ERROR("http_client_post->header malloc fail\n");
        return FAILURE_RET;
    }
    memset(url, 0, 256);
    HAL_Snprintf(url, 256, "https://http-cn-sh2.iot.ucloud.cn/topic/%s",topic);


    ret = http_client_common(http_client_post, url, 443, ca_crt, HTTP_POST, http_data_post,5000);
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("HTTP_POST error\n");
        goto end;
    }

    HAL_Free(url);
    
    ret = http_client_recv_data(http_client_post, 5000, http_data_post);
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("http_client_recv_data error\n");
        goto end;
    }
    LOG_DEBUG("response_buf:%s\n",http_data_post->response_buf);

    char *temp_message = LITE_json_value_of((char *)"Message", http_data_post->response_buf);
    if(NULL == temp_message)
    {
        LOG_ERROR("parse Message error\n");
        goto end;
    }
    LOG_DEBUG("%s\n", temp_message);    
    HAL_Free(temp_message);
end:    
    http_client_close(http_client_post);    
    HAL_Free(http_client_post->header);
    HAL_Free(http_client_post);
    HAL_Free(http_data_post->response_buf);
    HAL_Free(http_data_post->post_buf);
    HAL_Free(http_data_post);   
    return ret;
}




