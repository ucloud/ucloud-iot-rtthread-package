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

#ifndef C_SDK_UIOT_IMPORT_H_
#define C_SDK_UIOT_IMPORT_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <inttypes.h>

#include "uiot_defs.h"
#include "HAL_Timer_Platform.h"
#include "utils_net.h"

/**
 * @brief 创建互斥量
 *
 * @return 创建成功返回Mutex指针，创建失败返回NULL
 */
void *HAL_MutexCreate(void);

/**
 * @brief 销毁互斥量
 *
 * @param Mutex指针
 */
void HAL_MutexDestroy(_IN_ void *mutex);

/**
 * @brief 阻塞式加锁。如果互斥量已被另一个线程锁定和拥有，则调用该函数的线程将阻塞，直到该互斥量变为可用为止
 *
 * @param Mutex指针
 */
void HAL_MutexLock(_IN_ void *mutex);

/**
 * @brief 非阻塞式加锁。如果mutex参数所指定的互斥锁已经被锁定，调用函数将立即返回FAILURE，不会阻塞当前线程
 *
 * @param  Mutex指针
 * @return 如果成功获取锁，则返回SUCCESS，获取失败返回FAILURE
 */
IoT_Error_t HAL_MutexTryLock(_IN_ void *mutex);

/**
 * @brief 释放互斥量
 *
 * @param Mutex指针
 */
void HAL_MutexUnlock(_IN_ void *mutex);

/**
 * @brief 申请内存块
 *
 * @param size   申请的内存块大小
 * @return       申请成功返回指向内存首地址的指针，申请失败返回NULL
 */
void *HAL_Malloc(_IN_ uint32_t size);

/**
 * @brief 释放内存块
 *
 * @param ptr   指向要释放的内存块的指针
 */
void HAL_Free(_IN_ void *ptr);

/**
 * @brief 打印函数，向标准输出格式化打印一个字符串
 *
 * @param fmt   格式化字符串
 * @param ...   可变参数列表
 */
void HAL_Printf(_IN_ const char *fmt, ...);

/**
 * @brief 打印函数, 向内存缓冲区格式化打印一个字符串
 *
 * @param str   指向字符缓冲区的指针
 * @param len   缓冲区字符长度
 * @param fmt   格式化字符串
 * @param ...   可变参数列表
 * @return      实际写入缓冲区的字符长度
 */
int HAL_Snprintf(_OU_ char *str, _IN_ int len, _IN_ const char *fmt, ...);

/**
 * @brief 打印函数, 格式化输出字符串到指定buffer中
 *
 * @param [out] str: 用于存放写入字符串的buffer
 * @param [in] len:  允许写入的最大字符串长度
 * @param [in] fmt:  格式化字符串
 * @param [in] ap:   可变参数列表
 * @return 成功写入的字符串长度
 */
int HAL_Vsnprintf(_OU_ char *str, _IN_ int len, _IN_ const char *fmt, _IN_ va_list ap);

/**
 * @brief 检索自系统启动以来已运行的毫秒数.
 *
 * @return 返回毫秒数.
 */
uint64_t HAL_UptimeMs(void);

/**
 * @brief 休眠.
 *
 * @param ms 休眠的时长, 单位毫秒.
 */
void HAL_SleepMs(_IN_ uint32_t ms);

/**
 * @brief 获取产品序列号。从设备持久化存储（例如FLASH）中读取产品序列号。
 *
 * @param productSN  存放ProductSN的字符串缓冲区
 * @return           返回SUCCESS, 表示获取成功，FAILURE表示获取失败
 */
IoT_Error_t HAL_GetProductSN(_OU_ char productSN[IOT_PRODUCT_SN_LEN + 1]);

/**
 * @brief 获取产品密钥（动态注册）。从设备持久化存储（例如FLASH）中读取产品密钥。
 *
 * @param productSecret   存放ProductSecret的字符串缓冲区
 * @return                返回SUCCESS, 表示获取成功，FAILURE表示获取失败
 */
IoT_Error_t HAL_GetProductSecret(_OU_ char productSecret[IOT_PRODUCT_SECRET_LEN + 1]);

/**
 * @brief 获取设备序列号。从设备持久化存储（例如FLASH）中读取设备序列号。
 *
 * @param deviceSN   存放DeviceSN的字符串缓冲区
 * @return           返回SUCCESS, 表示获取成功，FAILURE表示获取失败
 */
IoT_Error_t HAL_GetDeviceSN(_OU_ char deviceSN[IOT_DEVICE_SN_LEN + 1]);

/**
 * @brief 获取设备密钥。从设备持久化存储（例如FLASH）中读取设备密钥。
 *
 * @param deviceSecret  存放DeviceSecret的字符串缓冲区
 * @return              返回SUCCESS, 表示获取成功，FAILURE表示获取失败
 */
IoT_Error_t HAL_GetDeviceSecret(_OU_ char deviceSecret[IOT_DEVICE_SECRET_LEN + 1]);

/**
 * @brief 设置产品序列号。将产品序列号烧写到设备持久化存储（例如FLASH）中，以备后续使用。
 *
 * @param pProductSN  指向待设置的产品序列号的指针
 * @return            返回SUCCESS, 表示设置成功，FAILURE表示设置失败
 */
IoT_Error_t HAL_SetProductSN(_IN_ const char *pProductSN);

/**
 * @brief 设置产品密钥。将产品密钥烧写到设备持久化存储（例如FLASH）中，以备后续使用。
 *
 * @param pProductSecret  指向待设置的产品密钥的指针
 * @return                返回SUCCESS, 表示设置成功，FAILURE表示设置失败
 */
IoT_Error_t HAL_SetProductSecret(_IN_ const char *pProductSecret);

/**
 * @brief 设置设备序列号。将设备序列号烧写到设备持久化存储（例如FLASH）中，以备后续使用。
 *
 * @param pDeviceSN  指向待设置的设备序列号的指针
 * @return           返回SUCCESS, 表示设置成功，FAILURE表示设置失败
 */
IoT_Error_t HAL_SetDeviceSN(_IN_ const char *pDeviceSN);

/**
 * @brief 设置设备密钥。将设备密钥烧写到设备持久化存储（例如FLASH）中，以备后续使用。
 *
 * @param pDeviceSecret  指向待设置的设备密钥的指针
 * @return               返回SUCCESS, 表示设置成功，FAILURE表示设置失败
 */
IoT_Error_t HAL_SetDeviceSecret(_IN_ const char *pDeviceSecret);

/**
 * 定义特定平台下的一个定时器结构体，需要在特定平台的代码中创建一个名为
 * HAL_Timer_Platform.h的头文件，在该头文件中定义Timer结构体。
 */
typedef struct Timer Timer;

/**
 * @brief 判断定时器时间是否已经过期
 *
 * @param timer     定时器结构体
 * @return          返回true, 表示定时器已过期
 */
bool HAL_Timer_Expired(_IN_ Timer *timer);

/**
 * @brief 根据timeout时间开启定时器计时, 单位: ms
 *
 * @param timer         定时器结构体
 * @param timeout_ms    超时时间, 单位:ms
 */
void HAL_Timer_Countdown_ms(_IN_ Timer *timer, _IN_ uint32_t timeout_ms);

/**
 * @brief 根据timeout时间开启定时器计时, 单位: s
 *
 * @param timer   定时器结构体
 * @param timeout 超时时间, 单位:s
 */
void HAL_Timer_Countdown(_IN_ Timer *timer, _IN_ uint32_t timeout);

/**
 * @brief 检查给定定时器剩余时间
 *
 * @param timer     定时器结构体
 * @return          定时器剩余时间, 单位: ms
 */
uint32_t HAL_Timer_Remain_ms(_IN_ Timer *timer);

/**
 * @brief 初始化定时器结构体
 *
 * @param timer 定时器结构体
 */
void HAL_Timer_Init(_IN_ Timer *timer);

/**
 * @brief 建立TLS连接
 *
 * @host        指定的TLS服务器网络地址
 * @port        指定的TLS服务器端口
 * @ca_crt      指向X.509证书的指针
 * @ca_crt_len  证书字节长度
 * @return      连接成功, 返回TLS连接句柄，连接失败，返回NULL
 */
uintptr_t HAL_TLS_Connect(_IN_ const char *host, _IN_ uint16_t port, _IN_ uint16_t authmode, _IN_ const char *ca_crt,
                          _IN_ size_t ca_crt_len);

/**
 * @brief 断开TLS连接, 并释放相关对象资源
 *
 * @param pParams TLS连接参数
 * @return        若断开成功返回SUCCESS，否则返回FAILURE
 */
int32_t HAL_TLS_Disconnect(_IN_ uintptr_t handle);

/**
 * @brief 向指定的TLS连接写入数据。此接口为同步接口, 如果在超时时间内写入了参数len指定长度的数据则立即返回, 否则在超时时间到时返回
 *
 * @param handle        TLS连接句柄
 * @param buf           指向数据发送缓冲区的指针
 * @param len           数据发送缓冲区的字节大小
 * @param timeout_ms    超时时间, 单位:ms
 * @return              <0: TLS写入错误; =0: TLS写超时, 且没有写入任何数据; >0: TLS成功写入的字节数
 */
int32_t HAL_TLS_Write(_IN_ uintptr_t handle, _IN_ unsigned char *buf, _IN_ size_t len, _IN_ uint32_t timeout_ms);

/**
 * @brief 通过TLS连接读数据
 *
 * @param handle        TLS连接句柄
 * @param buf           指向数据接收缓冲区的指针
 * @param len           数据接收缓冲区的字节大小
 * @param timeout_ms    超时时间, 单位:ms
 * @return              <0: TLS读取错误; =0: TLS读超时, 且没有读取任何数据; >0: TLS成功读取的字节数
 */
int32_t HAL_TLS_Read(_IN_ uintptr_t handle, _OU_ unsigned char *buf, _IN_ size_t len, _IN_ uint32_t timeout_ms);

/**
 * @brief   建立TCP连接。根据指定的HOST地址, 服务器端口号建立TCP连接, 返回对应的连接句柄。
 *
 * @host    指定的TCP服务器网络地址
 * @port    指定的TCP服务器端口
 * @return  连接成功, 返回TCP连接句柄，连接失败，返回NULL
 */
uintptr_t HAL_TCP_Connect(_IN_ const char *host, _IN_ uint16_t port);

/**
 * @brief 断开TCP连接, 并释放相关对象资源。
 *
 * @param fd  TCP连接句柄
 * @return    成功返回SUCCESS，失败返回FAILURE
 */
int32_t HAL_TCP_Disconnect(_IN_ uintptr_t fd);

/**
 * @brief 向指定的TCP连接写入数据。此接口为同步接口, 如果在超时时间内写入了参数len指定长度的数据则立即返回, 否则在超时时间到时返回。
 *
 * @param fd                TCP连接句柄
 * @param buf               指向数据发送缓冲区的指针
 * @param len               数据发送缓冲区的字节大小
 * @param timeout_ms        超时时间，单位: ms
 * @return                  <0: TCP写入错误; =0: TCP写超时, 且没有写入任何数据; >0: TCP成功写入的字节数
 */
int32_t HAL_TCP_Write(_IN_ uintptr_t fd, _IN_ unsigned char *buf, _IN_ size_t len, _IN_ uint32_t timeout_ms);

/**
 * @brief 从指定的TCP连接读取数据。此接口为同步接口, 如果在超时时间内读取到参数len指定长度的数据则立即返回, 否则在超时时间到时返回。
 *
 * @param fd                TCP连接句柄
 * @param buf               指向数据接收缓冲区的指针
 * @param len               数据接收缓冲区的字节大小
 * @param timeout_ms        超时时间，单位: ms
 * @return                  <0: TCP读取错误; =0: TCP读超时, 且没有读取任何数据; >0: TCP成功读取的字节数
 */
int32_t HAL_TCP_Read(_IN_ uintptr_t fd, _OU_ unsigned char *buf, _IN_ size_t len, _IN_ uint32_t timeout_ms);

int HAL_AT_Read(_IN_ utils_network_pt pNetwork, _OU_ unsigned char *buffer, _IN_ size_t len);

int HAL_AT_Write(_IN_ unsigned char *buffer, _IN_ size_t len);

int HAL_AT_TCP_Disconnect(void);

int HAL_AT_TCP_Connect(_IN_ void * pNetwork, _IN_ const char *host, _IN_ uint16_t port); 

int HAL_AT_Write_Tcp(_IN_ utils_network_pt pNetwork, _IN_ unsigned char *buffer, _IN_ size_t len);

int HAL_AT_Read_Tcp(_IN_ utils_network_pt pNetwork, _IN_ unsigned char *buffer, _IN_ size_t len);

#if defined(__cplusplus)
}
#endif
#endif  /* C_SDK_UIOT_IMPORT_H_ */
