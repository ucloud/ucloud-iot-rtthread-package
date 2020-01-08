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

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include "uiot_defs.h"
#include "utils_httpc.h"
#include "utils_net.h"
#include "utils_timer.h"
#include "utils_md5.h"

#define MIN_TIMEOUT                   (100)
#define MAX_RETRY_COUNT               (600)

#define HTTP_CLIENT_READ_BUF_SIZE     (1024)          /* read payload */
#define HTTP_CLIENT_READ_HEAD_SIZE    (32)            /* read header */
#define HTTP_CLIENT_SEND_BUF_SIZE     (1024)          /* send */
#define HTTP_CLIENT_REQUEST_BUF_SIZE  (300)           /* send */
#define HTTP_CLIENT_MAX_URL_LEN       (256)
#define HTTP_RETRIEVE_MORE_DATA       (1)             /**< More data needs to be retrieved. */
#define HTTP_CLIENT_CHUNK_SIZE        (1024)
static int _utils_parse_url(const char *url, char *host, char *path) {
    char *host_ptr = (char *) strstr(url, "://");
    uint32_t host_len = 0;
    uint32_t path_len;
    /* char *port_ptr; */
    char *path_ptr;
    char *fragment_ptr;

    if (host_ptr == NULL) {
        return -1; /* URL is invalid */
    }
    host_ptr += 3;

    path_ptr = strchr(host_ptr, '/');
    if (NULL == path_ptr) {
        return -2;
    }

    host_len = path_ptr - host_ptr;

    memcpy(host, host_ptr, host_len);
    host[host_len] = '\0';
    fragment_ptr = strchr(host_ptr, '#');
    if (fragment_ptr != NULL) {
        path_len = fragment_ptr - path_ptr;
    } else {
        path_len = strlen(path_ptr);
    }

    memcpy(path, path_ptr, path_len);
    path[path_len] = '\0';

    return SUCCESS_RET;
}

static int _utils_fill_tx_buffer(http_client_t *client, unsigned char *send_buf, int *send_idx, char *buf, uint32_t len) {
    int ret;
    int cp_len;
    int writen_len = 0;
    int idx = *send_idx;

    if (len == 0) {
        len = strlen(buf);
    }
    do {
        if ((HTTP_CLIENT_SEND_BUF_SIZE - idx) >= len) {
            cp_len = len;
        } else {
            cp_len = HTTP_CLIENT_SEND_BUF_SIZE - idx;
        }

        memcpy(send_buf + idx, buf + writen_len, cp_len);
        idx += cp_len;
        writen_len += cp_len;
        len -= cp_len;

        if (idx == HTTP_CLIENT_SEND_BUF_SIZE) {
            ret = client->net.write(&client->net, send_buf, HTTP_CLIENT_SEND_BUF_SIZE, 5000);
            if (ret < 0) {
                return (ret);
            } else if (ret != HTTP_CLIENT_SEND_BUF_SIZE) {
                return (ret == 0) ? ERR_HTTP_CLOSED : ERR_HTTP_CONN_ERROR;
            }
            idx -= ret;
        }
    } while (len);

    *send_idx = idx;
    return SUCCESS_RET;
}

static int _http_send_header(http_client_t *client, char *host, const char *path, int method,
                             http_client_data_t *client_data) {
    int len;
    unsigned char send_buf[HTTP_CLIENT_SEND_BUF_SIZE] = {0};
    char buf[HTTP_CLIENT_REQUEST_BUF_SIZE] = {0};
    char *pMethod = (method == HTTP_GET) ? "GET" : (method == HTTP_POST) ? "POST" :
            (method == HTTP_PUT) ? "PUT" : (method == HTTP_DELETE) ? "DELETE" :
            (method == HTTP_HEAD) ? "HEAD" : "";
    int ret;

    /* Send request */
    memset(send_buf, 0, HTTP_CLIENT_SEND_BUF_SIZE);
    len = 0; /* Reset send buffer */

    HAL_Snprintf(buf, sizeof(buf), "%s %s HTTP/1.1\r\nHost: %s\r\n", pMethod, path, host); /* Write request */

    ret = _utils_fill_tx_buffer(client, send_buf, &len, buf, strlen(buf));
    if (ret < 0) {
        /* LOG_ERROR("Could not write request"); */
        return ERR_HTTP_CONN_ERROR;
    }

    /* Add user header information */
    if (client->header) {
        _utils_fill_tx_buffer(client, send_buf, &len, (char *) client->header, strlen(client->header));
    }

    if (client_data->post_buf != NULL) {
        HAL_Snprintf(buf, sizeof(buf), "Content-Length: %d\r\n", client_data->post_buf_len);
        _utils_fill_tx_buffer(client, send_buf, &len, buf, strlen(buf));

        if (client_data->post_content_type != NULL) {
            HAL_Snprintf(buf, sizeof(buf), "Content-Type: %s\r\n", client_data->post_content_type);
            _utils_fill_tx_buffer(client, send_buf, &len, buf, strlen(buf));
        }
    }

    /* Close headers */
    _utils_fill_tx_buffer(client, send_buf, &len, "\r\n", 0);

    ret = client->net.write(&client->net, send_buf, len, 5000);
    if (ret <= 0) {
        LOG_ERROR("ret = client->net.write() = %d", ret);
        return (ret == 0) ? ERR_HTTP_CLOSED : ERR_HTTP_CONN_ERROR;
    }

    return SUCCESS_RET;
}

int _http_send_user_data(http_client_t *client, http_client_data_t *client_data, uint32_t timeout_ms) {
    int ret = 0;

    if (client_data->post_buf && client_data->post_buf_len) {
        ret = client->net.write(&client->net, (unsigned char *) client_data->post_buf, client_data->post_buf_len, timeout_ms);
        LOG_DEBUG("client_data->post_buf: %s, ret is %d", client_data->post_buf, ret);
        if (ret <= 0) {
            return (ret == 0) ? ERR_HTTP_CLOSED : ERR_HTTP_CONN_ERROR; /* Connection was closed by server */
        }
    }

    return SUCCESS_RET;
}

/* 0 on success, err code on failure */
static int _http_recv(http_client_t *client, unsigned char *buf, int max_len, int *p_read_len,
                      uint32_t timeout_ms) {
    int ret = 0;
    Timer timer;

    init_timer(&timer);
    countdown_ms(&timer, timeout_ms);

    *p_read_len = 0;

    ret = client->net.read(&client->net, buf, max_len, left_ms(&timer));

    if (ret > 0) {
        *p_read_len = ret;
        return 0;
    } else if (ret == 0) {
        /* timeout */
        return FAILURE_RET;
    } else {
        return ERR_HTTP_CONN_ERROR;
    }
}


static int _utils_check_deadloop(int len, Timer *timer, int ret, unsigned int *dead_loop_count,
                                 unsigned int *extend_count) {
    /* if timeout reduce to zero, it will be translated into NULL for select function in TLS lib */
    /* it would lead to indefinite behavior, so we avoid it */
    if (left_ms(timer) < MIN_TIMEOUT) {
        (*extend_count)++;
        countdown_ms(timer, MIN_TIMEOUT);
    }

    /* if it falls into deadloop before reconnected to internet, we just quit*/
    if ((0 == len) && (0 == left_ms(timer)) && (FAILURE_RET == ret)) {
        (*dead_loop_count)++;
        if (*dead_loop_count > MAX_RETRY_COUNT) {
            LOG_ERROR("deadloop detected, exit");
            return ERR_HTTP_CONN_ERROR;
        }
    } else {
        *dead_loop_count = 0;
    }

    /*if the internet connection is fixed during the loop, the download stream might be disconnected. we have to quit */
    if ((0 == len) && (*extend_count > 2 * MAX_RETRY_COUNT) && (FAILURE_RET == ret)) {
        LOG_ERROR("extend timer for too many times, exit");
        return ERR_HTTP_CONN_ERROR;
    }
    return SUCCESS_RET;
}

static int _utils_fill_rx_buf(int *recv_count, int len_to_write_to_response_buf, http_client_data_t *client_data,
                              unsigned char *data) {
    int count = *recv_count;
    if (count + len_to_write_to_response_buf < client_data->response_buf_len - 1) {
        memcpy(client_data->response_buf + count, data, len_to_write_to_response_buf);
        count += len_to_write_to_response_buf;
        client_data->response_buf[count] = '\0';
        client_data->retrieve_len -= len_to_write_to_response_buf;
        *recv_count = count;
        return SUCCESS_RET;
    } else {
        memcpy(client_data->response_buf + count, data, client_data->response_buf_len - 1 - count);
        client_data->response_buf[client_data->response_buf_len - 1] = '\0';
        client_data->retrieve_len -= (client_data->response_buf_len - 1 - count);
        return HTTP_RETRIEVE_MORE_DATA;
    }
}

static int _http_get_response_body(http_client_t *client, unsigned char *data, int data_len_actually_received,
                                   uint32_t timeout_ms, http_client_data_t *client_data) {
    int written_response_buf_len = 0;
    int len_to_write_to_response_buf = 0;
    Timer timer;

    init_timer(&timer);
    countdown_ms(&timer, timeout_ms);

    /* Receive data */
    /* LOG_DEBUG("Current data: %s", data); */

    client_data->is_more = 1;

    /* the header is not received finished */
    if (client_data->response_content_len == -1 && client_data->is_chunked == 0) {
        /* can not enter this if */
        LOG_ERROR("header is not received yet");
        return ERR_HTTP_CONN_ERROR;
    }

    while (1) {
        unsigned int dead_loop_count = 0;
        unsigned int extend_count = 0;
        do {
            int res;
            /* move previous fetched data into response_buf */
            len_to_write_to_response_buf = Min(data_len_actually_received, client_data->retrieve_len);
            res = _utils_fill_rx_buf(&written_response_buf_len, len_to_write_to_response_buf, client_data, data);
            if (HTTP_RETRIEVE_MORE_DATA == res) {
                return HTTP_RETRIEVE_MORE_DATA;
            }

            /* get data from internet and put into "data" buf temporary */
            if (client_data->retrieve_len) {
                int ret;
                int max_len_to_receive = Min(HTTP_CLIENT_CHUNK_SIZE - 1,
                                             client_data->response_buf_len - 1 - written_response_buf_len);
                max_len_to_receive = Min(max_len_to_receive, client_data->retrieve_len);

                ret = _http_recv(client, data, max_len_to_receive, &data_len_actually_received, left_ms(&timer));
                if (ret == ERR_HTTP_CONN_ERROR) {
                    return ret;
                }
                LOG_DEBUG("Total- remaind Payload: %d Bytes; currently Read: %d Bytes", client_data->retrieve_len,
                          data_len_actually_received);

                ret = _utils_check_deadloop(data_len_actually_received, &timer, ret, &dead_loop_count,
                                            &extend_count);
                if (ERR_HTTP_CONN_ERROR == ret) {
                    return ret;
                }
            }
        } while (client_data->retrieve_len);
        client_data->is_more = 0;
        break;
    }

    return SUCCESS_RET;
}

static int _http_parse_response_header(http_client_t *client, char *data, int len, uint32_t timeout_ms,
                                       http_client_data_t *client_data) {
    int crlf_pos;
    Timer timer;
    char *tmp_ptr, *ptr_body_end;
    int new_trf_len, ret;
    char *crlf_ptr;

    init_timer(&timer);
    countdown_ms(&timer, timeout_ms);

    client_data->response_content_len = -1;

    /* http client response */
    /* <status-line> HTTP/1.1 200 OK(CRLF)

       <headers> ...(CRLF)

       <blank line> (CRLF)

      [<response-body>] */
    crlf_ptr = strstr(data, "\r\n");
    if (crlf_ptr == NULL) {
        LOG_ERROR("\r\n not found");
        return ERR_HTTP_UNRESOLVED_DNS;
    }

    crlf_pos = crlf_ptr - data;
    data[crlf_pos] = '\0';
    client->response_code = atoi(data + 9);
    LOG_DEBUG("Reading headers: %s", data);
    memmove(data, &data[crlf_pos + 2], len - (crlf_pos + 2) + 1); /* Be sure to move NULL-terminating char as well */
    len -= (crlf_pos + 2);       /* remove status_line length */
    client_data->is_chunked = 0;

    /*If not ending of response body*/
    /* try to read more header again until find response head ending "\r\n\r\n" */
    while (NULL == (ptr_body_end = strstr(data, "\r\n\r\n"))) {
        /* try to read more header */
        ret = _http_recv(client, (unsigned char *) (data + len), HTTP_CLIENT_READ_HEAD_SIZE, &new_trf_len,
                         left_ms(&timer));
        if (ret == ERR_HTTP_CONN_ERROR) {
            return ret;
        }
        len += new_trf_len;
        data[len] = '\0';
    }

    /* parse response_content_len */
    if (NULL != (tmp_ptr = strstr(data, "Content-Length"))) {
        client_data->response_content_len = atoi(tmp_ptr + strlen("Content-Length: "));
        client_data->retrieve_len = client_data->response_content_len;
    } else {
        LOG_ERROR("Could not parse header");
        return ERR_HTTP_CONN_ERROR;
    }

    /* remove header length */
    /* len is Had read body's length */
    /* if client_data->response_content_len != 0, it is know response length */
    /* the remain length is client_data->response_content_len - len */
    len = len - (ptr_body_end + 4 - data);
    memmove(data, ptr_body_end + 4, len + 1);
    client_data->response_received_len += len;
    return _http_get_response_body(client, (unsigned char *) data, len, left_ms(&timer), client_data);
}

static int _http_connect(http_client_t *client) {
    int retry_max = 3;
    int retry_cnt = 1;
    int retry_interval = 1000;
    int rc = -1;

    do {
        client->net.handle = 0;
        LOG_DEBUG("calling TCP or TLS connect HAL for [%d/%d] iteration", retry_cnt, retry_max);

        rc = client->net.connect(&client->net);
        if (0 != rc) {
            client->net.disconnect(&client->net);
            LOG_ERROR("TCP or TLS connect failed, rc = %d", rc);
            HAL_SleepMs(retry_interval);
            continue;
        } else {
            LOG_DEBUG("rc = client->net.connect() = %d, success @ [%d/%d] iteration", rc, retry_cnt, retry_max);
            break;
        }
    } while (++retry_cnt <= retry_max);

    return SUCCESS_RET;
}

int _http_send_request(http_client_t *client, const char *url, HTTP_Request_Method method,
                       http_client_data_t *client_data, uint32_t timeout_ms) {
    int ret = ERR_HTTP_CONN_ERROR;

    if (0 == client->net.handle) {
        return -1;
    }

    int rc;
    char host[HTTP_CLIENT_MAX_URL_LEN] = {0};
    char path[HTTP_CLIENT_MAX_URL_LEN] = {0};

    rc = _utils_parse_url(url, host, path);
    if (rc != SUCCESS_RET) {
        return rc;
    }

    ret = _http_send_header(client, host, path, method, client_data);
    if (ret != 0) {
        return -2;
    }

    if (method == HTTP_POST || method == HTTP_PUT) {
        ret = _http_send_user_data(client, client_data,timeout_ms);
        if (ret < 0) {
            ret = -3;
        }
    }

    return ret;
}

static int _http_client_recv_response(http_client_t *client, uint32_t timeout_ms, http_client_data_t *client_data) {
    int read_len = 0, ret = ERR_HTTP_CONN_ERROR;
    char buf[HTTP_CLIENT_READ_BUF_SIZE] = {0};
    Timer timer;

    init_timer(&timer);
    countdown_ms(&timer, timeout_ms);

    if (0 == client->net.handle) {
        LOG_ERROR("no connection have been established");
        return ret;
    }

    if (client_data->is_more) {
        client_data->response_buf[0] = '\0';
        ret = _http_get_response_body(client, (unsigned char *) buf, read_len, left_ms(&timer), client_data);
    } else {
        client_data->is_more = 1;
        /* try to read header */
        ret = _http_recv(client, (unsigned char *) buf, HTTP_CLIENT_READ_HEAD_SIZE, &read_len, left_ms(&timer));
        if (ret != 0) {
            return ret;
        }

        buf[read_len] = '\0';

        if (read_len) {
            ret = _http_parse_response_header(client, buf, read_len, left_ms(&timer), client_data);
        }
    }

    return ret;
}

int http_client_connect(http_client_t *client, const char *url, int port, const char *ca_crt) {
    if (client->net.handle != 0) {
        LOG_ERROR("http client has connected to host!");
        return ERR_HTTP_CONN_ERROR;
    }

    int rc;
    char host[HTTP_CLIENT_MAX_URL_LEN] = {0};
    char path[HTTP_CLIENT_MAX_URL_LEN] = {0};

    rc = _utils_parse_url(url, host, path);
    if (rc != SUCCESS_RET) {
        return rc;
    }

    rc = utils_net_init(&client->net, host, port, SSL_CA_VERIFY_REQUIRED, ca_crt);
    if (rc != SUCCESS_RET) {
        return rc;
    }

    rc = _http_connect(client);
    if (rc != SUCCESS_RET) {
        LOG_ERROR("_http_connect error, rc = %d", rc);
        http_client_close(client);
    } else {
        LOG_DEBUG("http client connect success");
    }
    return rc;
}

int http_client_common(http_client_t *client, const char *url, int port, const char *ca_crt,
                       HTTP_Request_Method method, http_client_data_t *client_data, uint32_t timeout_ms) {
    int rc;

    if (client->net.handle == 0) {
        rc = http_client_connect(client, url, port, ca_crt);
        if (rc != SUCCESS_RET) {
            return rc;
        }
    }

    rc = _http_send_request(client, url, method, client_data, timeout_ms);
    if (rc != SUCCESS_RET) {
        LOG_ERROR("http_send_request error, rc = %d", rc);
        http_client_close(client);
        return rc;
    }

    return SUCCESS_RET;
}

int http_client_recv_data(http_client_t *client, uint32_t timeout_ms, http_client_data_t *client_data) {
    int rc;
    Timer timer;

    init_timer(&timer);
    countdown_ms(&timer, (unsigned int) timeout_ms);

    if ((NULL != client_data->response_buf)
        && (0 != client_data->response_buf_len)) {
        rc = _http_client_recv_response(client, left_ms(&timer), client_data);
        if (rc < 0) {
            LOG_ERROR("_http_client_recv_response is error, rc = %d", rc);
            http_client_close(client);
            return rc;
        }
    }
    return SUCCESS_RET;
}

void http_client_close(http_client_t *client) {
    if (client->net.handle > 0) {
        client->net.disconnect(&client->net);
    }
    client->net.handle = 0;
    LOG_INFO("client disconnected");
}

void http_client_file_md5(char* file_path, char *output)
{
    iot_md5_context ctx;
    
    utils_md5_init(&ctx);
    utils_md5_starts(&ctx);
    
    char *buffer = (char *)HAL_Malloc(1024);
    if (NULL == buffer) {
        return;
    }
    memset(buffer,0,1024);
    uint32_t count = 0;
    FILE *fp = fopen(file_path, "rb+");
    if(NULL == fp)
    {
        return;
    }

    while((count = fread(buffer,1,1024,fp))){
        utils_md5_update(&ctx, (unsigned char *)buffer, count);
    }
    utils_md5_finish_hb2hex(&ctx, output);
    utils_md5_free(&ctx);
    fclose(fp);
    HAL_Free(buffer);
}

#ifdef __cplusplus
}
#endif
