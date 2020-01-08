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

//based on Alibaba c-sdk
/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#ifndef C_SDK_UTILS_NET_H_
#define C_SDK_UTILS_NET_H_

#include <stdint.h>
#include <stddef.h>

#ifdef SUPPORT_AT_CMD
#include "stm32f7xx_hal.h"
#include "at_client.h"
#endif

/**
 * @brief The structure of network connection(TCP or SSL).
 *   The user has to allocate memory for this structure.
 */

struct utils_network;
typedef struct utils_network utils_network_t, *utils_network_pt;

typedef enum
{
    SSL_CA_VERIFY_NONE = 0,         
    SSL_CA_VERIFY_OPTIONAL = 1,
    SSL_CA_VERIFY_REQUIRED = 2,
    SSL_CA_VERIFY_UNSET = 3, 
}SSL_AUTH_MODE;

#ifdef SUPPORT_AT_CMD
struct utils_network {
    const char *pHostAddress;
    uint16_t port;

    /**< uart for send at cmd */
    at_client *pclient;

    uint16_t authmode;

    /**< connection handle: 0, NOT connection; NOT 0, handle of the connection */
    uintptr_t handle;

    /**< Read data from server function pointer. */
    int (*read)(utils_network_pt,unsigned char *, size_t, uint32_t);

    /**< Send data to server function pointer. */
    int (*write)(utils_network_pt,unsigned char *, size_t, uint32_t);

    /**< Disconnect the network */
    int (*disconnect)(utils_network_pt);

    /**< Establish the network */
    int (*connect)(utils_network_pt);
};
#else
struct utils_network {
    const char *pHostAddress;
    uint16_t port;
    uint16_t ca_crt_len;

    uint16_t authmode;

    /**< NULL, TCP connection; NOT NULL, SSL connection */
    const char *ca_crt;
    /**< connection handle: 0, NOT connection; NOT 0, handle of the connection */
    uintptr_t handle;

    /**< Read data from server function pointer. */
    int (*read)(utils_network_pt,unsigned char *, size_t, uint32_t);

    /**< Send data to server function pointer. */
    int (*write)(utils_network_pt,unsigned char *, size_t, uint32_t);

    /**< Disconnect the network */
    int (*disconnect)(utils_network_pt);

    /**< Establish the network */
    int (*connect)(utils_network_pt);
};
#endif
int utils_net_read(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms);
int utils_net_write(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms);
int utils_net_disconnect(utils_network_pt pNetwork);
int utils_net_connect(utils_network_pt pNetwork);
int utils_net_init(utils_network_pt pNetwork, const char *host, uint16_t port, uint16_t authmode, const char *ca_crt);

#endif /* C_SDK_UTILS_NET_H_ */


