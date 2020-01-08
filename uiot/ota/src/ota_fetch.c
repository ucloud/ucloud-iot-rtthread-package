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

#include "uiot_export_ota.h"
#include "uiot_import.h"
#include "ota_internal.h"
#include "ca.h"
#include "utils_httpc.h"

typedef struct {

    const char *url;
    http_client_t http;             /* http client */
    http_client_data_t http_data;   /* http client data */

} OTA_Http_Client;


void *ofc_init(const char *url)
{
    FUNC_ENTRY;

    OTA_Http_Client *h_ofc;

    if (NULL == (h_ofc = HAL_Malloc(sizeof(OTA_Http_Client)))) {
        LOG_ERROR("allocate for h_odc failed");
        FUNC_EXIT_RC(NULL);
    }

    memset(h_ofc, 0, sizeof(OTA_Http_Client));

    /* set http request-header parameter */
    h_ofc->http.header = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n" \
                         "Accept-Encoding: gzip, deflate\r\n";

    h_ofc->url = url;

    FUNC_EXIT_RC(h_ofc);
}


int32_t ofc_connect(void *handle)
{
    FUNC_ENTRY;

    OTA_Http_Client * h_ofc = (OTA_Http_Client *)handle;

#ifdef SUPPORT_TLS
    int port = 443;
    const char *ca_crt = iot_https_ca_get();
#else
    int port = 80;
    const char *ca_crt = NULL;
#endif

    int32_t rc = http_client_common(&h_ofc->http, h_ofc->url, port, ca_crt, HTTP_GET, &h_ofc->http_data, 5000);

    FUNC_EXIT_RC(rc);
}


int32_t ofc_fetch(void *handle, char *buf, uint32_t buf_len, uint32_t timeout_s)
{
    FUNC_ENTRY;

    int diff;
    OTA_Http_Client * h_ofc = (OTA_Http_Client *)handle;

    h_ofc->http_data.response_buf = buf;
    h_ofc->http_data.response_buf_len = buf_len;
    diff = h_ofc->http_data.response_content_len - h_ofc->http_data.retrieve_len;
    
    int rc = http_client_recv_data(&h_ofc->http, timeout_s * 1000, &h_ofc->http_data);
    if (SUCCESS_RET != rc) {
        if (rc == ERR_HTTP_NOT_FOUND)
            FUNC_EXIT_RC(ERR_OTA_FILE_NOT_EXIST);
        
        if (rc == ERR_HTTP_AUTH_ERROR)
            FUNC_EXIT_RC(ERR_OTA_FETCH_AUTH_FAIL);
        
        if (rc == ERR_HTTP_TIMEOUT)
            FUNC_EXIT_RC(ERR_OTA_FETCH_TIMEOUT);
        
        FUNC_EXIT_RC(rc);
    }

    FUNC_EXIT_RC(h_ofc->http_data.response_content_len - h_ofc->http_data.retrieve_len - diff);
}


int ofc_deinit(void *handle)
{
    FUNC_ENTRY;

    OTA_Http_Client *h_ofc = (OTA_Http_Client *)handle;

    http_client_close(&h_ofc->http);
    if (NULL != handle) {
        HAL_Free(handle);
    }

    FUNC_EXIT_RC(SUCCESS_RET);
}
