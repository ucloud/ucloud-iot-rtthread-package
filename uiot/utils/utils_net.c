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

#include <stdio.h>
#include <string.h>
#include "uiot_defs.h"
#include "utils_net.h"
#include "uiot_import.h"


#ifdef SUPPORT_TLS

static int read_ssl(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    return HAL_TLS_Read((uintptr_t)pNetwork->handle, buffer, len, timeout_ms);
}

static int write_ssl(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    return HAL_TLS_Write((uintptr_t)pNetwork->handle, buffer, len, timeout_ms);
}

static int disconnect_ssl(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    HAL_TLS_Disconnect((uintptr_t)pNetwork->handle);
    pNetwork->handle = 0;

    return SUCCESS_RET;
}

static int connect_ssl(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    if (0 != (pNetwork->handle = (intptr_t)HAL_TLS_Connect(
            pNetwork->pHostAddress,
            pNetwork->port,
            pNetwork->authmode,
            pNetwork->ca_crt,
            pNetwork->ca_crt_len))) {
        return SUCCESS_RET;
    }
    else {
        return FAILURE_RET;
    }
}

/*** TCP connection ***/
static int read_tcp(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    return HAL_TCP_Read((uintptr_t)pNetwork->handle, buffer, len, timeout_ms);
}


static int write_tcp(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    return HAL_TCP_Write((uintptr_t)pNetwork->handle, buffer, len, timeout_ms);
}

static int disconnect_tcp(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    HAL_TCP_Disconnect(pNetwork->handle);
    pNetwork->handle = (uintptr_t)(-1);
    return SUCCESS_RET;
}

static int connect_tcp(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    pNetwork->handle = HAL_TCP_Connect(pNetwork->pHostAddress, pNetwork->port);
    if (pNetwork->handle == (uintptr_t)(-1)) {
        return FAILURE_RET;
    }

    return SUCCESS_RET;
}

#elif SUPPORT_AT_CMD
/* connect TCP by AT cmd through uart */
static int at_read_tcp(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    return HAL_AT_Read_Tcp(pNetwork, buffer, len);
}

static int at_write_tcp(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    return HAL_AT_Write_Tcp(pNetwork, buffer, len);
}

static int at_disconnect_tcp(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    return HAL_AT_TCP_Disconnect(pNetwork);
}

static int at_connect_tcp(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    return HAL_AT_TCP_Connect(pNetwork,pNetwork->pHostAddress, pNetwork->port);
}

#else
/*** TCP connection ***/
static int read_tcp(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    return HAL_TCP_Read((uintptr_t)pNetwork->handle, buffer, len, timeout_ms);
}


static int write_tcp(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    return HAL_TCP_Write((uintptr_t)pNetwork->handle, buffer, len, timeout_ms);
}

static int disconnect_tcp(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    HAL_TCP_Disconnect(pNetwork->handle);
    pNetwork->handle = (uintptr_t)(-1);
    return SUCCESS_RET;
}

static int connect_tcp(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    pNetwork->handle = HAL_TCP_Connect(pNetwork->pHostAddress, pNetwork->port);
    if (pNetwork->handle == (uintptr_t)(-1)) {
        return FAILURE_RET;
    }

    return SUCCESS_RET;
}
#endif  /* #ifdef SUPPORT_TLS */

/****** network interface ******/
int utils_net_read(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    int ret = 0;
#ifdef SUPPORT_TLS
    if (NULL != pNetwork->ca_crt) {
        ret = read_ssl(pNetwork, buffer, len, timeout_ms);
    }
    else
    {
        ret = read_tcp(pNetwork, buffer, len, timeout_ms);
    }
#elif SUPPORT_AT_CMD
        ret = at_read_tcp(pNetwork, buffer, len, timeout_ms);
#else
    if (NULL == pNetwork->ca_crt) {
        ret = read_tcp(pNetwork, buffer, len, timeout_ms);
    }   
#endif

    return ret;
}

int utils_net_write(utils_network_pt pNetwork,unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    int ret = 0;
#ifdef SUPPORT_TLS
    if (NULL != pNetwork->ca_crt) {
        ret = write_ssl(pNetwork, buffer, len, timeout_ms);
    }
    else
    {
        ret = write_tcp(pNetwork, buffer, len, timeout_ms);
    }
#elif SUPPORT_AT_CMD
        ret = at_write_tcp(pNetwork, buffer, len, timeout_ms);
#else
    if (NULL == pNetwork->ca_crt) {
        ret = write_tcp(pNetwork, buffer, len, timeout_ms);
    }
#endif

    return ret;
}

int utils_net_disconnect(utils_network_pt pNetwork)
{
    int ret = 0;
#ifdef SUPPORT_TLS
    if (NULL != pNetwork->ca_crt) {
        ret = disconnect_ssl(pNetwork);
    }
    else
    {
        ret = disconnect_tcp(pNetwork);
    }
#elif SUPPORT_AT_CMD
        ret = at_disconnect_tcp(pNetwork);
#else
    if (NULL == pNetwork->ca_crt) {
        ret = disconnect_tcp(pNetwork);
    }
#endif

    return  ret;
}

int utils_net_connect(utils_network_pt pNetwork)
{
    int ret = 0;
#ifdef SUPPORT_TLS
    if (NULL != pNetwork->ca_crt) {
        ret = connect_ssl(pNetwork);
    }
    else
    {
        ret = connect_tcp(pNetwork);
    }
#elif SUPPORT_AT_CMD
        ret = at_connect_tcp(pNetwork);
#else
    if (NULL == pNetwork->ca_crt) {
        ret = connect_tcp(pNetwork);
    }
#endif

    return ret;
}

int utils_net_init(utils_network_pt pNetwork, const char *host, uint16_t port, uint16_t authmode, const char *ca_crt)
{
    if (!pNetwork || !host) {
        LOG_ERROR("parameter error! pNetwork=%p, host = %p", pNetwork, host);
        return FAILURE_RET;
    }
    pNetwork->pHostAddress = host;
    pNetwork->port = port;
#ifndef SUPPORT_AT_CMD
    pNetwork->authmode = authmode;
    pNetwork->ca_crt = ca_crt;

    if (NULL == ca_crt) {
        pNetwork->ca_crt_len = 0;
    } else {
        pNetwork->ca_crt_len = strlen(ca_crt);
    }
#endif 

    pNetwork->handle = 0;
    pNetwork->read = utils_net_read;
    pNetwork->write = utils_net_write;
    pNetwork->disconnect = utils_net_disconnect;
    pNetwork->connect = utils_net_connect;

    return SUCCESS_RET;
}

