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

#ifndef C_SDK_UTILS_HTTPC_H_
#define C_SDK_UTILS_HTTPC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "utils_net.h"

typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_HEAD
} HTTP_Request_Method;

/** @brief   This structure defines the http_client_t structure.  */
typedef struct {
    int                 remote_port;    /**< HTTP or HTTPS port. */
    utils_network_t     net;
    int                 response_code;  /**< Response code. */
    char               *header;         /**< Custom header. */
    char               *auth_user;      /**< Username for basic authentication. */
    char               *auth_password;  /**< Password for basic authentication. */
} http_client_t;

/** @brief   This structure defines the HTTP data structure.  */
typedef struct {
    int     is_more;                /**< Indicates if more data needs to be retrieved. */
    int     is_chunked;             /**< Response data is encoded in portions/chunks.*/
    int     retrieve_len;           /**< Content length to be retrieved. */
    int     response_content_len;   /**< Response content length. */
    int     response_received_len;  /**< Response have received length. */
    int     post_buf_len;           /**< Post data length. */
    int     response_buf_len;       /**< Response buffer length. */
    char   *post_content_type;      /**< Content type of the post data. */
    unsigned char   *post_buf;      /**< User data to be posted. */
    char   *response_buf;           /**< Buffer to store the response data. */
} http_client_data_t;


int http_client_connect(http_client_t *client, const char *url, int port, const char *ca_crt);

int http_client_common(http_client_t *client, const char *url, int port, const char *ca_crt,
                       HTTP_Request_Method method, http_client_data_t *client_data, uint32_t timeout_ms);

int http_client_recv_data(http_client_t *client, uint32_t timeout_ms, http_client_data_t *client_data);

void http_client_close(http_client_t *client);

int _http_send_user_data(http_client_t *client, http_client_data_t *client_data, uint32_t timeout_ms);

void http_client_file_md5(char* file_path, char *output);

#ifdef __cplusplus
}
#endif
#endif /* C_SDK_UTILS_HTTPC_H_ */
