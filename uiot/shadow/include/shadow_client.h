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

#ifndef IOT_SHADOW_CLIENT_H_
#define IOT_SHADOW_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "mqtt_client.h"


/* 在任意给定时间内, 处于appending状态的请求最大个数 */
#define MAX_APPENDING_REQUEST_AT_ANY_GIVEN_TIME                     (10)

/* 一个method的最大长度 */
#define MAX_SIZE_OF_METHOD                                          (10)

/* 一个仅包含method字段的JSON文档的最大长度 */
#define MAX_SIZE_OF_JSON_WITH_METHOD                                (MAX_SIZE_OF_METHOD + 20)

/* 接收云端返回的JSON文档的buffer大小 */
#define CLOUD_IOT_JSON_RX_BUF_LEN                                   (UIOT_MQTT_RX_BUF_LEN + 1)

/* 最大等待时间 */
#define MAX_WAIT_TIME_SEC   1
#define MAX_WAIT_TIME_MS    1000

/**
 * @brief 请求响应返回的类型
 */
typedef enum {
    ACK_NONE = -3,      // 请求超时
    ACK_TIMEOUT = -2,   // 请求超时
    ACK_REJECTED = -1,  // 请求拒绝
    ACK_ACCEPTED = 0    // 请求接受
} RequestAck;

/**
 * @brief 操作云端设备文档可以使用的三种方式
 */
typedef enum {
    GET,                    // 获取云端设备文档
    UPDATE,                 // 更新或创建云端设备文档
    UPDATE_AND_RESET_VER,   // 更新同时重置版本号
    DELETE,                 // 删除部分云端设备文档
    DELETE_ALL,             // 删除全部云端设备文档中的属性,不需要一个个添加需要删除的属性
    REPLY_CONTROL_UPDATE,   // 设备处理完服务端的control消息后回应的update消息
    REPLY_CONTROL_DELETE,   // 设备处理完服务端的control消息后回应的delete消息
} Method;

/**
 * @brief JSON文档中支持的数据类型
 */
typedef enum {
    JINT32,     // 32位有符号整型
    JINT16,     // 16位有符号整型
    JINT8,      // 8位有符号整型
    JUINT32,    // 32位无符号整型
    JUINT16,    // 16位无符号整型
    JUINT8,     // 8位无符号整型
    JFLOAT,     // 单精度浮点型
    JDOUBLE,    // 双精度浮点型
    JBOOL,      // 布尔型
    JSTRING,    // 字符串
    JOBJECT     // JSON对象
} JsonDataType;

/**
 * @brief 定义设备的某个属性, 实际就是一个JSON文档节点
 */
typedef struct _JSONNode {
    char         *key;    // 该JSON节点的Key
    void         *data;   // 该JSON节点的Value
    JsonDataType type;    // 该JSON节点的数据类型
} DeviceProperty;

/**
 * @brief 每次文档请求响应的回调函数
 *
 * @param pClient        ShadowClient对象
 * @param method         文档操作方式
 * @param requestAck     请求响应类型
 * @param pJsonDocument  云端响应返回的文档
 * @param userContext    用户数据
 *
 */
typedef void (*OnRequestCallback)(void *pClient, Method method, RequestAck requestAck, const char *pJsonDocument, void *userContext);

/**
 * @brief 文档操作请求的参数结构体定义
 */
typedef struct _RequestParam {
    Method                   method;                // 文档请求方式: GET, UPDATE, DELETE等

    List                    *property_delta_list;   // 该请求需要修改的属性

    uint32_t                 timeout_sec;           // 请求超时时间, 单位:s

    OnRequestCallback        request_callback;      // 请求回调方法

    void                     *user_context;         // 用户数据, 会通过回调方法OnRequestCallback返回

} RequestParams;

/**
 * @brief 设备属性处理回调函数
 *
 * @param pClient          ShadowClient对象
 * @param pParams          设备影子文档修改请求
 * @param pJsonValueBuffer 设备属性值
 * @param valueLength      设备属性值长度
 * @param DeviceProperty   设备属性结构体
 */
typedef void (*OnPropRegCallback)(void *pClient, RequestParams *pParams, char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty);

/**
 * @brief 该结构体用于保存已登记的设备属性及设备属性处理的回调方法
 */
typedef struct {

    void *property;                  // 设备属性

    OnPropRegCallback callback;      // 回调处理函数

} PropertyHandler;

typedef struct _ShadowInnerData {
    uint32_t version;                   //本地维护的影子文档的版本号
    List *request_list;                 //影子文档的修改请求
    List *property_list;                //本地维护的影子文档的属性值,期望值和回调处理函数
} ShadowInnerData;

typedef struct _Shadow {
    void *mqtt;
    const char *product_sn;
    const char *device_sn;
    void *request_mutex;
    void *property_mutex;
    ShadowInnerData inner_data;
} UIoT_Shadow;

/**
 * @brief 设备影子初始化
 *
 * @param pShadow       shadow client
 * @return              返回SUCCESS, 表示成功
 */
int uiot_shadow_init(UIoT_Shadow *pShadow);

/**
 * @brief 设备影子重置,主要是将设备影子中的队列归零
 *
 * @param pClient       shadow client
 * @return              返回SUCCESS, 表示成功
 */
void uiot_shadow_reset(void *pClient);

/**
 * @brief 处理请求队列中已经超时的请求
 * 
 * @param pShadow   shadow client
 */
void _handle_expired_request(UIoT_Shadow *pShadow);

/**
 * @brief 所有的云端设备文档操作请求, 通过该方法进行中转分发
 *
 * @param pShadow       shadow client
 * @param pParams       请求参数
 * @param pJsonDoc      请求文档
 * @param sizeOfBuffer  文档缓冲区大小
 * @return              返回SUCCESS, 表示成功
 */
int uiot_shadow_make_request(UIoT_Shadow *pShadow,char *pJsonDoc, size_t sizeOfBuffer, RequestParams *pParams);

/**
 * @brief 订阅设备影子topic
 *
 * @param pShadow                   shadow client
 * @param topicFilter               topic的名称
 * @param on_message_handler        topic的消息回调函数
 * @return                 返回SUCCESS, 表示成功
 */
int uiot_shadow_subscribe_topic(UIoT_Shadow *pShadow, char *topicFilter, OnMessageHandler on_message_handler);

/**
 * @brief 初始化一个修改设备影子的请求
 * @param handle        ShadowClient对象
 * @param method        修改类型
 * @param callback      请求回调函数
 * @param timeout_sec   超时时间
 * @return              返回NULL,表示初始化失败
 */
void* uiot_shadow_request_init(Method method, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context);

/**
 * @brief 从服务端获取设备影子文档
 *
 * @param handle        shadow handle
 * @param message       返回的消息
 * @param pUserdata     跟随回调函数返回
 */
void topic_request_result_handler(void *pClient, MQTTMessage *message, void *pUserdata);

/**
 * @brief 从服务端获取设备影子文档
 *
 * @param handle        shadow handle
 * @param message       返回的消息
 * @param pUserdata     跟随回调函数返回
 */
void topic_sync_handler(void *pClient, MQTTMessage *message, void *pUserdata);


#ifdef __cplusplus
}
#endif

#endif /* IOT_SHADOW_CLIENT_H_ */
