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

#ifndef C_SDK_UTILS_MD5_H_
#define C_SDK_UTILS_MD5_H_

#include "uiot_import.h"

typedef struct {
    uint32_t total[2];          /*!< number of bytes processed  */
    uint32_t state[4];          /*!< intermediate digest state  */
    unsigned char buffer[64];   /*!< data block being processed */
} iot_md5_context;


 /**
 * @brief 初始化MD5上下文
 *
 * @param ctx   MD5上下文指针
 */
void utils_md5_init(iot_md5_context *ctx);

/**
 * @brief 清空MD5上下文
 *
 * @param ctx   MD5上下文指针
 */
void utils_md5_free(iot_md5_context *ctx);

/**
 * @brief 拷贝MD5上下文
 *
 * @param dst   目标MD5上下文
 * @param src   源MD5上下文
 */
void utils_md5_clone(iot_md5_context *dst,
                     const iot_md5_context *src);

/**
 * @brief 设置MD5上下文
 *
 * @param ctx   MD5上下文指针
 */
void utils_md5_starts(iot_md5_context *ctx);

/**
 * @brief MD5过程缓冲区
 *
 * @param ctx MD5上下文指针
 * @param input    输入数据
 * @param ilen     输入数据长度
 */
void utils_md5_update(iot_md5_context *ctx, const unsigned char *input, size_t ilen);

/**
 * @brief          MD5数据
 *
 * @param ctx      MD5上下文指针
 * @param output   MD5校验和结果
 */
void utils_md5_finish(iot_md5_context *ctx, unsigned char output[16]);

/* 内部使用 */
void utils_md5_process(iot_md5_context *ctx, const unsigned char data[64]);

/**
 * @brief          Output = MD5( input buffer )
 *
 * @param input    输入数据
 * @param ilen     输入数据长度
 * @param output   MD5校验和结果
 */
void utils_md5(const unsigned char *input, size_t ilen, unsigned char output[16]);


int8_t utils_hb2hex(uint8_t hb);

void utils_md5_finish_hb2hex(void *md5, char *output_str);

#endif //C_SDK_UTILS_MD5_H_

