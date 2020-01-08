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

#include <stdint.h>
#include <string.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "uiot_import.h"
#include "uiot_defs.h"

#include "HAL_Timer_Platform.h"

#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

/**
 * @brief 用于保存SSL连接相关数据结构
 */
typedef struct {
    mbedtls_net_context socket_fd;        // socket文件描述符
    mbedtls_entropy_context entropy;      // 保存熵配置
    mbedtls_ctr_drbg_context ctr_drbg;    // 随机数生成器
    mbedtls_ssl_context ssl;              // 保存SSL基本数据
    mbedtls_ssl_config ssl_conf;          // SSL/TLS配置信息
    mbedtls_x509_crt ca_cert;             // ca证书信息
    mbedtls_x509_crt client_cert;         // 客户端证书信息
    mbedtls_pk_context private_key;       // 客户端私钥信息
} TLSDataParams;

/**
 * @brief 释放mbedtls开辟的内存
 */
static void _free_mbedtls(TLSDataParams *pParams) {
    mbedtls_net_free(&(pParams->socket_fd));
    mbedtls_x509_crt_free(&(pParams->client_cert));
    mbedtls_x509_crt_free(&(pParams->ca_cert));
    mbedtls_pk_free(&(pParams->private_key));
    mbedtls_ssl_free(&(pParams->ssl));
    mbedtls_ssl_config_free(&(pParams->ssl_conf));
    mbedtls_ctr_drbg_free(&(pParams->ctr_drbg));
    mbedtls_entropy_free(&(pParams->entropy));

    HAL_Free(pParams);
}

/**
 * @brief mbedtls库初始化
 *
 * 1. 执行mbedtls库相关初始化函数
 * 2. 随机数生成器
 * 3. 加载CA证书
 *
 * @param pDataParams       TLS连接相关数据结构
 * @param pConnectParams    TLS证书密钥相关
 * @return                  返回SUCCESS, 表示成功
 */
static int _mbedtls_client_init(TLSDataParams *pDataParams, const char *ca_crt, size_t ca_crt_len) {
    int ret;
    mbedtls_net_init(&(pDataParams->socket_fd));
    mbedtls_ssl_init(&(pDataParams->ssl));
    mbedtls_ssl_config_init(&(pDataParams->ssl_conf));
    mbedtls_ctr_drbg_init(&(pDataParams->ctr_drbg));
    mbedtls_x509_crt_init(&(pDataParams->ca_cert));
    mbedtls_x509_crt_init(&(pDataParams->client_cert));
    mbedtls_pk_init(&(pDataParams->private_key));

    LOG_DEBUG("Seeding the random number generator...");
    mbedtls_entropy_init(&(pDataParams->entropy));
    // 随机数, 增加custom参数, 目前为NULL
    if ((ret = mbedtls_ctr_drbg_seed(&(pDataParams->ctr_drbg), mbedtls_entropy_func,
                                     &(pDataParams->entropy), NULL, 0)) != 0) {
        LOG_ERROR("failed! mbedtls_ctr_drbg_seed returned -0x%x\n", -ret);
        return ERR_SSL_INIT_FAILED;
    }

    LOG_DEBUG("Loading the CA root certificate ...");
    if (ca_crt != NULL) {
        if ((ret = mbedtls_x509_crt_parse(&(pDataParams->ca_cert), (const unsigned char *) ca_crt,
                                          (ca_crt_len + 1 )))) {
            LOG_ERROR("failed!  mbedtls_x509_crt_parse returned -0x%x while parsing root cert\n", -ret);
            return ERR_SSL_CERT_FAILED;
        }
    }
    return SUCCESS_RET;
}

/**
 * @brief 建立TCP连接
 *
 * @param socket_fd  Socket描述符
 * @param host       服务器主机名
 * @param port       服务器端口地址
 * @return 返回SUCCESS, 表示成功
 */
int _mbedtls_tcp_connect(mbedtls_net_context *socket_fd, const char *host, uint16_t port) {
    int ret = 0;
    char port_str[6];
    HAL_Snprintf(port_str, 6, "%d", port);
    if ((ret = mbedtls_net_connect(socket_fd, host, port_str, MBEDTLS_NET_PROTO_TCP)) != 0) {
        LOG_ERROR("failed! mbedtls_net_connect returned -0x%x\n", -ret);
        switch (ret) {
            case MBEDTLS_ERR_NET_SOCKET_FAILED:
                return ERR_TCP_SOCKET_FAILED;
            case MBEDTLS_ERR_NET_UNKNOWN_HOST:
                return ERR_TCP_UNKNOWN_HOST;
            default:
                return ERR_TCP_CONNECT_FAILED;
        }
    }
#if 0
    if ((ret = mbedtls_net_set_block(socket_fd)) != 0) {
        LOG_ERROR("failed! net_set_(non)block() returned -0x%x\n", -ret);
        return ERR_TCP_CONNECT_FAILED;
    }
#endif
    return SUCCESS_RET;
}

/**
 * @brief 在该函数中可对服务端证书进行自定义的校验
 *
 * 这种行为发生在握手过程中, 一般是校验连接服务器的主机名与服务器证书中的CN或SAN的域名信息是否一致
 * 不过, mbedtls库已经实现该功能, 可以参考函数 `mbedtls_x509_crt_verify_with_profile`
 *
 * @param hostname 连接服务器的主机名
 * @param crt x509格式的证书
 * @param depth
 * @param flags
 * @return
 */
int _server_certificate_verify(void *hostname, mbedtls_x509_crt *crt, int depth, uint32_t *flags) {
    return *flags;
}

uintptr_t HAL_TLS_Connect(_IN_ const char *host, _IN_ uint16_t port, _IN_ uint16_t authmode, _IN_ const char *ca_crt,
                          _IN_ size_t ca_crt_len) {
    int ret = 0;

    TLSDataParams *pDataParams = (TLSDataParams *) HAL_Malloc(sizeof(TLSDataParams));

    if ((ret = _mbedtls_client_init(pDataParams, ca_crt, ca_crt_len)) != SUCCESS_RET) {
        goto error;
    }

    LOG_INFO("Connecting to /%s/%d...", host, port);
    if ((ret = _mbedtls_tcp_connect(&(pDataParams->socket_fd), host, port)) != SUCCESS_RET) {
        goto error;
    }

    LOG_DEBUG("Setting up the SSL/TLS structure...");
    if ((ret = mbedtls_ssl_config_defaults(&(pDataParams->ssl_conf), MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        LOG_ERROR("failed! mbedtls_ssl_config_defaults returned -0x%x\n", -ret);
        goto error;
    }

    mbedtls_ssl_conf_verify(&(pDataParams->ssl_conf), _server_certificate_verify, (void *) host);

    mbedtls_ssl_conf_authmode(&(pDataParams->ssl_conf), authmode);

    mbedtls_ssl_conf_rng(&(pDataParams->ssl_conf), mbedtls_ctr_drbg_random, &(pDataParams->ctr_drbg));

    mbedtls_ssl_conf_ca_chain(&(pDataParams->ssl_conf), &(pDataParams->ca_cert), NULL);

    mbedtls_ssl_conf_read_timeout(&(pDataParams->ssl_conf), 10000);
    if ((ret = mbedtls_ssl_setup(&(pDataParams->ssl), &(pDataParams->ssl_conf))) != 0) {
        LOG_ERROR("failed! mbedtls_ssl_setup returned -0x%x\n", -ret);
        goto error;
    }

    // Set the hostname to check against the received server certificate and sni
    if ((ret = mbedtls_ssl_set_hostname(&(pDataParams->ssl), host)) != 0) {
        LOG_ERROR("failed! mbedtls_ssl_set_hostname returned %d\n", ret);
        goto error;
    }

    LOG_DEBUG("SSL state connect : %d ", pDataParams->ssl.state);
    mbedtls_ssl_set_bio(&(pDataParams->ssl), &(pDataParams->socket_fd), mbedtls_net_send, mbedtls_net_recv,
                        mbedtls_net_recv_timeout);
    LOG_DEBUG("SSL state connect : %d ", pDataParams->ssl.state);

    LOG_DEBUG("Performing the SSL/TLS handshake...");
    while ((ret = mbedtls_ssl_handshake(&(pDataParams->ssl))) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            LOG_ERROR("failed! mbedtls_ssl_handshake returned -0x%x\n", -ret);
            if (ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
                LOG_ERROR("Unable to verify the server's certificate");
            }
            goto error;
        }
    }

    if ((ret = mbedtls_ssl_get_verify_result(&(pDataParams->ssl))) != 0) {
        LOG_ERROR("mbedtls_ssl_get_verify_result failed returned 0x%04x\n", -ret);
        goto error;
    }

    //mbedtls_ssl_conf_read_timeout(&(pDataParams->ssl_conf), 100);

    LOG_INFO("connected with /%s/%d...", host, port);

    return (uintptr_t) pDataParams;

    error:
    _free_mbedtls(pDataParams);
    return 0;
}

int32_t HAL_TLS_Disconnect(_IN_ uintptr_t handle) {
    if ((uintptr_t) NULL == handle) {
        LOG_DEBUG("handle is NULL");
        return FAILURE_RET;
    }
    TLSDataParams *pParams = (TLSDataParams *) handle;
    int ret = 0;
    do {
        ret = mbedtls_ssl_close_notify(&(pParams->ssl));
    } while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    _free_mbedtls(pParams);
    return SUCCESS_RET;
}

int32_t HAL_TLS_Write(_IN_ uintptr_t handle, _IN_ unsigned char *buf, _IN_ size_t len, _IN_ uint32_t timeout_ms) {
    Timer timer;
    HAL_Timer_Init(&timer);
    HAL_Timer_Countdown_ms(&timer, (unsigned int) timeout_ms);
    size_t written_so_far;
    bool errorFlag = false;
    int write_rc = 0;

    TLSDataParams *pParams = (TLSDataParams *) handle;

    for (written_so_far = 0; written_so_far < len && !HAL_Timer_Expired(&timer); written_so_far += write_rc) {
        while (!HAL_Timer_Expired(&timer) &&
               (write_rc = mbedtls_ssl_write(&(pParams->ssl), (unsigned char *)(buf + written_so_far), len - written_so_far)) <= 0) {
            if (write_rc != MBEDTLS_ERR_SSL_WANT_READ && write_rc != MBEDTLS_ERR_SSL_WANT_WRITE) {
                LOG_ERROR("failed! mbedtls_ssl_write returned -0x%x\n", -write_rc);
                errorFlag = true;
                break;
            }
        }

        if (errorFlag) {
            break;
        }
    }

    if (errorFlag) {
        return ERR_SSL_WRITE_FAILED;
    }

    return written_so_far;
}

int32_t HAL_TLS_Read(_IN_ uintptr_t handle, _OU_ unsigned char *buf, _IN_ size_t len, _IN_ uint32_t timeout_ms) {
    Timer timer;
    HAL_Timer_Init(&timer);
    HAL_Timer_Countdown_ms(&timer, timeout_ms);
    size_t read_len = 0;

    TLSDataParams *pParams = (TLSDataParams *) handle;

    while (read_len < len) {
        int read_rc = 0;
        read_rc = mbedtls_ssl_read(&(pParams->ssl), (unsigned char *)(buf + read_len), len - read_len);

        if (read_rc > 0) {
            read_len += read_rc;
        } else if (read_rc == 0 || (read_rc != MBEDTLS_ERR_SSL_WANT_WRITE
                                    && read_rc != MBEDTLS_ERR_SSL_WANT_READ && read_rc != MBEDTLS_ERR_SSL_TIMEOUT)) {
            LOG_ERROR("failed! mbedtls_ssl_read returned -0x%x\n", -read_rc);
            return ERR_SSL_READ_FAILED;
        }

        if (HAL_Timer_Expired(&timer)) {
            break;
        }
    }

    return read_len;
}

#ifdef __cplusplus
}
#endif
