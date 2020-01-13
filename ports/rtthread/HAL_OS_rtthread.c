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
#include <stdarg.h>
     
#include "uiot_defs.h"
#include "stdint.h"
#include <rtthread.h>

#define DEBUG_DEV_INFO_USED
// 测试时可在此处填入设备四元组
#ifdef DEBUG_DEV_INFO_USED
/* 产品序列号 */
static char sg_product_sn[IOT_PRODUCT_SN_LEN + 1] = "YOUR_PRODUCT_SN";
/* 产品密钥 */
static char sg_product_secret[IOT_PRODUCT_SECRET_LEN + 1] = "PRODUCT_SECRET";
/* 设备序列号 */
static char sg_device_sn[IOT_DEVICE_SN_LEN + 1] = "YOUR_DEVICE_SN";
/* 设备密钥 */
static char sg_device_secret[IOT_DEVICE_SECRET_LEN + 1] = "DEVICE_SECRET";
#endif

static int g_mutexnum = 0;

void *HAL_MutexCreate(void) 
{
    char name[RT_NAME_MAX];
    rt_snprintf(name, RT_NAME_MAX, "mutex%d", g_mutexnum);
    g_mutexnum++;
    return rt_mutex_create(name, RT_IPC_FLAG_FIFO);
}

void HAL_MutexDestroy(_IN_ void *mutex) 
{
    rt_mutex_delete((rt_mutex_t)mutex);
    return;
}

void HAL_MutexLock(_IN_ void *mutex) 
{
    rt_mutex_take((rt_mutex_t)mutex, 0xFFFF);
    return;
}

void HAL_MutexUnlock(_IN_ void *mutex) 
{   
    rt_mutex_release((rt_mutex_t)mutex);
    return;
}

void *HAL_Malloc(_IN_ uint32_t size)
{
    return rt_malloc(size);
}

void HAL_Free(_IN_ void *ptr) 
{
    rt_free(ptr);
}

#define HAL_OS_LOG_MAXLEN   1024
void HAL_Printf(_IN_ const char *fmt, ...) 
{
    va_list args;
    char log_buf[HAL_OS_LOG_MAXLEN];
    
    va_start(args, fmt);
    rt_vsnprintf(log_buf, HAL_OS_LOG_MAXLEN, fmt, args);
    va_end(args);
    printf("%s", log_buf);
}

int HAL_Snprintf(_IN_ char *str, _IN_ const int len, _IN_ const char *fmt, ...) {
    va_list args;
    int rc;

    va_start(args, fmt);
    rc = vsnprintf(str, len, fmt, args);
    va_end(args);

    return rc;
}

int HAL_Vsnprintf(_OU_ char *str, _IN_ const int len, _IN_ const char *format, _IN_ va_list ap) {
    return vsnprintf(str, len, format, ap);
}

void HAL_SleepMs(uint32_t ms)
{
    rt_thread_mdelay(ms);
}

IoT_Error_t HAL_GetProductSN(_OU_ char productSN[IOT_PRODUCT_SN_LEN + 1]) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(sg_product_sn);
    memset(productSN, 0x0, IOT_PRODUCT_SN_LEN + 1);
    strncpy(productSN, sg_product_sn, len);
    return SUCCESS_RET;
#else
    HAL_Printf("HAL_GetProductSN is not implement");
    return FAILURE_RET;
#endif
}

IoT_Error_t HAL_GetProductSecret(_OU_ char productSecret[IOT_PRODUCT_SECRET_LEN + 1]) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(sg_product_secret);
    memset(productSecret, 0x0, IOT_PRODUCT_SECRET_LEN + 1);
    strncpy(productSecret, sg_product_secret, len);
    return SUCCESS_RET;
#else
    Log_e("HAL_GetProductSecret is not implement");
    return FAILURE_RET;
#endif
}

IoT_Error_t HAL_GetDeviceSN(_OU_ char deviceSN[IOT_DEVICE_SN_LEN + 1]) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(sg_device_sn);
    memset(deviceSN, 0x0, IOT_DEVICE_SN_LEN + 1);
    strncpy(deviceSN, sg_device_sn, len);
    return SUCCESS_RET;
#else
    HAL_Printf("HAL_GetDeviceSN is not implement");
    return FAILURE_RET;
#endif
}

IoT_Error_t HAL_GetDeviceSecret(_OU_ char deviceSecret[IOT_DEVICE_SECRET_LEN + 1]) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(sg_device_secret);
    memset(deviceSecret, 0x0, IOT_DEVICE_SECRET_LEN + 1);
    strncpy(deviceSecret, sg_device_secret, len);
    return SUCCESS_RET;
#else
    HAL_Printf("HAL_GetDeviceSecret is not implement");
    return FAILURE_RET;
#endif
}

IoT_Error_t HAL_SetProductSN(_IN_ const char *pProductSN) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(pProductSN);
    if (len > IOT_PRODUCT_SN_LEN) {
        return FAILURE_RET;
    }

    memset(sg_product_sn, 0x0, IOT_PRODUCT_SN_LEN + 1);
    strncpy(sg_product_sn, pProductSN, len);

    return SUCCESS_RET;
#else
    HAL_Printf("HAL_SetProductSN is not implement");
    return FAILURE_RET;
#endif
}

IoT_Error_t HAL_SetProductSecret(_IN_ const char *pProductSecret) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(pProductSecret);
    if (len > IOT_PRODUCT_SECRET_LEN) {
        return FAILURE_RET;
    }

    memset(sg_product_secret, 0x0, IOT_PRODUCT_SECRET_LEN + 1);
    strncpy(sg_product_secret, pProductSecret, len);

    return SUCCESS_RET;
#else
    HAL_Printf("HAL_SetProductSecret is not implement");
    return FAILURE_RET;
#endif

}

IoT_Error_t HAL_SetDeviceSN(_IN_ const char *pDeviceSN) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(pDeviceSN);
    if (len > IOT_DEVICE_SN_LEN) {
        return FAILURE_RET;
    }

    memset(sg_device_sn, 0x0, IOT_DEVICE_SN_LEN + 1);
    strncpy(sg_device_sn, pDeviceSN, len);

    return SUCCESS_RET;
#else
    HAL_Printf("HAL_SetDeviceSN is not implement");
    return FAILURE_RET;
#endif
}

IoT_Error_t HAL_SetDeviceSecret(_IN_ const char *pDeviceSecret) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(pDeviceSecret);
    if (len > IOT_DEVICE_SECRET_LEN) {
        return FAILURE_RET;
    }

    memset(sg_device_secret, 0x0, IOT_DEVICE_SECRET_LEN + 1);
    strncpy(sg_device_secret, pDeviceSecret, len);

    return SUCCESS_RET;
#else
    HAL_Printf("HAL_SetDeviceSecret is not implement");
    return FAILURE_RET;
#endif
}

