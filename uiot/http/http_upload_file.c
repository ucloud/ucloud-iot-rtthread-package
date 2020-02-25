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

int IOT_GET_URL_AND_AUTH(const char *product_sn, const char *device_sn, const char *device_sercret, char *file_name, char *upload_buffer, uint32_t upload_buffer_len, char *md5, char *authorization, char *put_url)
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
    
    HAL_Snprintf((char *)http_data_post->post_buf, 1024, "{\"ProductSN\":\"%s\",\"DeviceSN\":\"%s\","   \
                                                         "\"FileName\":\"%s\",\"FileSize\":%d,\"MD5\":\"%s\",\"Content-Type\":\"plain/text\"}", \
                                                         product_sn, device_sn, file_name, upload_buffer_len, md5);
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
    HAL_Snprintf(http_client_post->header, 1024, "Content-Type: application/json\r\nAuthorization:%s\r\n",mac_output_char);

    const char *ca_crt = iot_https_ca_get();
    char *url = (char *)"https://file-cn-sh2.iot.ucloud.cn/api/v1/url";

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

    char *temp_auth = LITE_json_value_of((char *)"Authorization", http_data_post->response_buf);
    if(NULL == temp_auth)
    {
        LOG_ERROR("parse Authorization error\n");
        goto end;
    }
    strncpy(authorization,temp_auth,strlen(temp_auth));
    authorization[strlen(temp_auth)+1] = '\0';
    LOG_DEBUG("authorization:%s\n",authorization);
    HAL_Free(temp_auth);
    
    char *temp_url = LITE_json_value_of((char *)"URL", http_data_post->response_buf);
    if(NULL == temp_url)
    {
        LOG_ERROR("parse URL error\n");
        goto end;
    }
    strncpy(put_url,temp_url,strlen(temp_url));
    put_url[strlen(temp_url)+1] = '\0';
    LOG_DEBUG("put_url:%s\n",put_url);    
    HAL_Free(temp_url);

end:    
    http_client_close(http_client_post);    
    HAL_Free(http_client_post->header);
    HAL_Free(http_client_post);
    HAL_Free(http_data_post->response_buf);
    HAL_Free(http_data_post->post_buf);
    HAL_Free(http_data_post);
    return ret;
}

int IOT_HTTP_UPLOAD_FILE(char *file_name, char *upload_buffer, uint32_t upload_buffer_len, char *md5, char *authorization, char *url, uint32_t timeout_ms)
{
    int ret = SUCCESS_RET;
    http_client_t *http_client_put = (http_client_t *)HAL_Malloc(sizeof(http_client_t));
    if(NULL == http_client_put)
    {
        LOG_ERROR("http_client_put malloc fail\n");
        return FAILURE_RET;
    }
    http_client_data_t *http_data_put = (http_client_data_t *)HAL_Malloc(sizeof(http_client_data_t));
    if(NULL == http_data_put)
    {
        HAL_Free(http_client_put);
        LOG_ERROR("http_data_put malloc fail\n");
        return FAILURE_RET;
    }
    
    memset(http_client_put, 0, sizeof(http_client_t));
    memset(http_data_put, 0, sizeof(http_client_data_t));

    http_data_put->response_buf = (char *)HAL_Malloc(512);
    if(NULL == http_data_put)
    {
        HAL_Free(http_client_put);
        HAL_Free(http_data_put);
        LOG_ERROR("http_data_put->response_buf malloc fail\n");
        return FAILURE_RET;
    }
    
    memset(http_data_put->response_buf, 0, 512);
    http_data_put->response_buf_len = 512;

    http_client_put->header = (char *)HAL_Malloc(1024);
    if(NULL == http_data_put)
    {
        HAL_Free(http_client_put);        
        HAL_Free(http_data_put->response_buf);
        HAL_Free(http_data_put);
        LOG_ERROR("http_client_put->header malloc fail\n");
        return FAILURE_RET;
    }
    memset(http_client_put->header, 0, 1024);
    HAL_Snprintf(http_client_put->header, 1024, "Authorization:%s\r\nContent-Type:plain/text\r\nContent-MD5:%s\r\n",authorization,md5);
    LOG_DEBUG("header:%s\n", http_client_put->header);

    http_data_put->post_content_type = (char *)"plain/text";

    http_data_put->post_buf = (unsigned char *)HAL_Malloc(upload_buffer_len + 1);
    if(NULL == http_data_put->post_buf)
    {
        HAL_Free(http_data_put->response_buf);        
        HAL_Free(http_data_put);
        HAL_Free(http_client_put->header);
        HAL_Free(http_client_put);
        LOG_ERROR("http_data_put->post_buf malloc fail\n");    
        return FAILURE_RET;
    }
    memset(http_data_put->post_buf, 0, upload_buffer_len + 1);
    
    strncpy((char *)http_data_put->post_buf, upload_buffer, upload_buffer_len);
    http_data_put->post_buf[upload_buffer_len]= '\0'; 
    http_data_put->post_buf_len = upload_buffer_len;    
    const char *ca_crt = iot_https_ca_get();

    ret = http_client_common(http_client_put, url, 443, ca_crt, HTTP_PUT, http_data_put, timeout_ms);
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("http_client_common error\n");
        goto end;
    }

    ret = http_client_recv_data(http_client_put, timeout_ms, http_data_put);
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("http_client_recv_data error\n");
        goto end;
    }

    LOG_DEBUG("content_len:%d response_received_len:%d\n",http_data_put->response_content_len, http_data_put->response_received_len);
    LOG_DEBUG("response_buf:%s\n",http_data_put->response_buf);
end:    
    http_client_close(http_client_put);
    HAL_Free(http_data_put->post_buf);
    HAL_Free(http_data_put->response_buf);
    HAL_Free(http_data_put);        
    HAL_Free(http_client_put->header);
    HAL_Free(http_client_put);
    return ret;
}


