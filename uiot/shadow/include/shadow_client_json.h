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


#ifndef IOT_SHADOW_CLIENT_JSON_H_
#define IOT_SHADOW_CLIENT_JSON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>

#include "uiot_import.h"
#include "shadow_client.h"

/* 回复消息中的消息字段 */
#define METHOD_FIELD                        "Method"
#define PAYLOAD_RESULT_FIELD                "Payload.RetCode"
#define PAYLOAD_STATE_REPORTED_FIELD        "Payload.State.Reported"
#define PAYLOAD_STATE_DESIRED_FIELD         "Payload.State.Desired"

/* 设备影子文档中的字段 */
#define STATE_FIELD                         "State"
#define STATE_REPORTED_FIELD                "State.Reported"
#define STATE_DESIRED_FIELD                 "State.Desired"
#define METADATA_FIELD                      "Metadata"
#define METADATA_REPORTED_FIELD             "Metadata.Reported"
#define METADATA_DESIRED_FIELD              "Metadata.Desired"
#define VERSION_FIELD                       "Version"
#define TIMESTAMP_FIELD                     "Timestamp"

/* 消息类型字段 */
#define METHOD_CONTROL                      "control"
#define METHOD_GET                          "get"
#define METHOD_UPDATE                       "update"
#define METHOD_DELETE                       "delete"
#define METHOD_REPLY                        "reply"
#define METHOD_GET_REPLY                    "get_reply"

/**
 * @brief 检查函数snprintf的返回值
 *
 * @param returnCode       函数snprintf的返回值
 * @param maxSizeOfWrite   可写最大字节数
 * @return                 返回ERR_JSON, 表示出错; 返回ERR_JSON_BUFFER_TRUNCATED, 表示截断
 */
int _check_snprintf_return(int32_t returnCode, size_t maxSizeOfWrite);

/**
 * 将一个JSON节点写入到JSON串中
 *
 * @param jsonBuffer    JSON串
 * @param sizeOfBuffer  可写入大小
 * @param pKey          JSON节点的key
 * @param pData         JSON节点的value
 * @param type          JSON节点value的数据类型
 * @return              返回SUCCESS, 表示成功
 */
int put_json_node(char *jsonBuffer, size_t sizeOfBuffer, const char *pKey, void *pData, JsonDataType type);

/**
 * @brief 从JSON文档中解析出report字段
 *
 * @param pJsonDoc              待解析的JSON文档
 * @param pType                 输出tyde字段
 * @return                      返回true, 表示解析成功
 */
bool parse_shadow_payload_state_reported_state(char *pJsonDoc, char **pState);


/**
 * @brief 从JSON文档中解析出desired字段
 *
 * @param pJsonDoc              待解析的JSON文档
 * @param pType                 输出tyde字段
 * @return                      返回true, 表示解析成功
 */
bool parse_shadow_payload_state_desired_state(char *pJsonDoc, char **pState);

/**
 * @brief 从JSON文档中解析出type字段
 *
 * @param pJsonDoc              待解析的JSON文档
 * @param pType                 输出tyde字段
 * @return                      返回true, 表示解析成功
 */
bool parse_shadow_method_type(char *pJsonDoc, char **pType);

/**
 * @brief 从JSON文档中解析出recode字段
 *
 * @param pJsonDoc              待解析的JSON文档
 * @param pType                 输出tyde字段
 * @return                      返回true, 表示解析成功
 */
bool parse_shadow_payload_retcode_type(char *pJsonDoc, uint32_t *pRetCode);

/**
 * @brief 从JSON文档中解析出version字段
 *
 * @param pJsonDoc              待解析的JSON文档
 * @param pType                 输出tyde字段
 * @return                      返回true, 表示解析成功
 */
bool parse_version_num(char *pJsonDoc, uint32_t *pVersionNumber);

/**
 * @brief 从JSON文档中解析出reported字段
 *
 * @param pJsonDoc              待解析的JSON文档
 * @param pType                 输出tyde字段
 * @return                      返回true, 表示解析成功
 */
bool parse_shadow_state_reported_type(char *pJsonDoc, char **pType);

/**
 * @brief 从JSON文档中解析出desired字段
 *
 * @param pJsonDoc              待解析的JSON文档
 * @param pType                 输出tyde字段
 * @return                      返回true, 表示解析成功
 */
bool parse_shadow_state_desired_type(char *pJsonDoc, char **pType);

/**
 * @brief 从JSON文档中解析出state字段
 *
 * @param pJsonDoc              待解析的JSON文档
 * @param pType                 输出tyde字段
 * @return                      返回true, 表示解析成功
 */
bool parse_shadow_state_type(char *pJsonDoc, char **pType);

/**
 * @brief 为GET和DELETE请求构造一个只带有clientToken字段的JSON文档
 *
 * @param tokenNumber shadow的token值，函数内部每次执行完会自增
 * @param pJsonBuffer 存储JSON文档的字符串缓冲区
 */
void build_empty_json(uint32_t *tokenNumber, char *pJsonBuffer);

/**
 * @brief 如果JSON文档中的key与某个设备属性的key匹配的话, 则更新该设备属性, 该设备属性的值不能为OBJECT类型
 *
 * @param pJsonDoc       JSON文档
 * @param pProperty      设备属性
 * @return               返回true, 表示成功
 */
char *find_value_if_key_match(char *pJsonDoc, DeviceProperty *pProperty);

#ifdef __cplusplus
}
#endif

#endif //IOT_SHADOW_CLIENT_JSON_H_
