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


#ifndef C_SDK_UIOT_EXPORT_MQTT_H_
#define C_SDK_UIOT_EXPORT_MQTT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 服务质量等级。服务质量等级表示PUBLISH消息分发的质量等级。可参考MQTT协议说明 4.7
 */
typedef enum _QoS {
    QOS0 = 0,    // 至多分发一次
    QOS1 = 1,    // 至少分发一次, 消息的接收者需回复PUBACK报文
    QOS2 = 2     // 仅分发一次, 目前暂不支持
} QoS;

/**
 * @brief 发布或接收已订阅消息的结构体定义
 */
typedef struct {
    QoS         			qos;          // MQTT 服务质量等级
    uint8_t     			retained;     // RETAIN 标识位
    uint8_t     			dup;          // DUP 标识位
    uint16_t    			id;           // MQTT 消息标识符

    const char  			*topic;       // MQTT topic
    size_t      			topic_len;    // topic 长度

    void        			*payload;     // MQTT 消息负载
    size_t      			payload_len;  // MQTT 消息负载长度
} MQTTMessage;

typedef MQTTMessage PublishParams;

#define DEFAULT_PUB_PARAMS {QOS0, 0, 0, 0, NULL, 0, NULL, 0}

/**
 * @brief 接收已订阅消息的回调函数定义
 */
typedef void (*OnMessageHandler)(void *pClient, MQTTMessage *message, void *pUserData);

/**
 * @brief 订阅主题的结构体定义
 */
typedef struct {
    QoS                     qos;                    // QOS服务质量标识
    OnMessageHandler        on_message_handler;     // 接收已订阅消息的回调函数
    void                    *user_data;             // 用户数据, 通过callback返回
} SubscribeParams;

#define DEFAULT_SUB_PARAMS {QOS0, NULL, NULL}

typedef enum {

    /* 未定义事件 */
    MQTT_EVENT_UNDEF = 0,

    /* MQTT 断开连接 */
    MQTT_EVENT_DISCONNECT = 1,

    /* MQTT 重连 */
    MQTT_EVENT_RECONNECT = 2,

    /* 订阅成功 */
    MQTT_EVENT_SUBSCRIBE_SUCCESS = 3,

    /* 订阅超时 */
    MQTT_EVENT_SUBSCRIBE_TIMEOUT = 4,

    /* 订阅失败 */
    MQTT_EVENT_SUBSCRIBE_NACK = 5,

    /* 取消订阅成功 */
    MQTT_EVENT_UNSUBSCRIBE_SUCCESS = 6,

    /* 取消订阅超时 */
    MQTT_EVENT_UNSUBSCRIBE_TIMEOUT = 7,

    /* 取消订阅失败 */
    MQTT_EVENT_UNSUBSCRIBE_NACK = 8,

    /* 发布成功 */
    MQTT_EVENT_PUBLISH_SUCCESS = 9,

    /* 发布超时 */
    MQTT_EVENT_PUBLISH_TIMEOUT = 10,

    /* 发布失败 */
    MQTT_EVENT_PUBLISH_NACK = 11,

    /* SDK订阅的topic收到后台push消息 */
    MQTT_EVENT_PUBLISH_RECEIVED = 12,

} MQTTEventType;

typedef struct {
    MQTTEventType  event_type;
    void           *msg;
} MQTTEventMsg;

/**
 * @brief 定义了函数指针的数据类型. 当相关事件发生时，将调用这种类型的函数.
 *
 * @param context, the program context
 * @param pClient, the MQTT client
 * @param msg, the event message.
 *
 * @return none
 */
typedef void (*MQTTEventHandlerFun)(void *pClient, void *context, MQTTEventMsg *msg);


/* The structure of MQTT event handle */
typedef struct {
    MQTTEventHandlerFun      h_fp;
    void                     *context;
} MQTTEventHandler;

typedef struct {
	/**
	 * 设备基础信息
	 */
    char 						*product_sn;	         // 产品序列号
    char 						*device_sn;			     // 设备序列号
    char                        *product_secret;         // 产品密钥 用于动态注册,不填则认为是静态注册
    char                        *device_secret;          // 设备密钥 用于静态注册，动态注册时可以不填

    uint32_t					command_timeout;		 // 发布订阅信令读写超时时间 ms
    uint32_t					keep_alive_interval;	 // 心跳周期, 单位: s

    uint8_t         			clean_session;			 // 清理会话标志位

    uint8_t                   	auto_connect_enable;     // 是否开启自动重连 1:启用自动重连 0：不启用自动重连  建议为1

    MQTTEventHandler            event_handler;           // 事件回调

} MQTTInitParams;

#define DEFAULT_MQTT_INIT_PARAMS { NULL, NULL, NULL, NULL, 2000, 240, 1, 1, {0}}

/**
 * @brief 构造MQTTClient并完成MQTT连接
 *
 * @param pParams MQTT协议连接接入与连接维持阶段所需要的参数
 *
 * @return        构造成功返回MQTT句柄，构造失败返回NULL
 */
void* IOT_MQTT_Construct(MQTTInitParams *pParams);

/**
 * @brief 关闭MQTT连接并销毁MQTTClient
 *
 * @param pClient MQTT句柄
 *
 * @return        返回SUCCESS, 表示销毁成功，返回FAILURE表示失败
 */
int IOT_MQTT_Destroy(void **pClient);

/**
 * @brief 在当前线程为底层MQTT客户端让出一定CPU执行时间
 *
 * 在这段时间内, MQTT客户端会用用处理消息接收, 以及发送PING报文, 监控网络状态
 *
 * @param pClient    MQTT句柄
 * @param timeout_ms Yield操作超时时间
 * @return           返回SUCCESS, 表示成功, 返回ERR_MQTT_ATTEMPTING_RECONNECT, 表示正在重连
 */
int IOT_MQTT_Yield(void *pClient, uint32_t timeout_ms);

/**
 * @brief 发布MQTT消息
 *
 * @param pClient   MQTT句柄
 * @param topicName 主题名
 * @param pParams   发布参数
 * @return < 0  :   表示失败
 *         >= 0 :   返回唯一的packet id 
 */
int IOT_MQTT_Publish(void *pClient, char *topicName, PublishParams *pParams);

/**
 * @brief 订阅MQTT主题
 *
 * @param pClient     MQTT句柄
 * @param topicFilter 主题过滤器, 可参考MQTT协议说明 4.7
 * @param pParams     订阅参数
 * @return <  0 :     表示失败
 *         >= 0 :     返回唯一的packet id
 */
int IOT_MQTT_Subscribe(void *pClient, char *topicFilter, SubscribeParams *pParams);

/**
 * @brief 取消订阅已订阅的MQTT主题
 *
 * @param pClient      MQTT客户端结构体
 * @param topicFilter  主题过滤器, 可参考MQTT协议说明 4.7
 * @return <  0 :      表示失败
 *         >= 0 :      返回唯一的packet id
 */
int IOT_MQTT_Unsubscribe(void *pClient, char *topicFilter);

/**
 * @brief 客户端目前是否已连接
 *
 * @param pClient  MQTT Client结构体
 * @return         返回true, 表示客户端已连接，返回false表示断开连接
 */
bool IOT_MQTT_IsConnected(void *pClient);

/**
 * @brief 构造MQTTClient动态注册
 *
 * @param  pParams MQTT协议连接接入与连接维持阶段所需要的参数
 *
 * @return 成功返回SUCCESS，否则返回错误码
 */
int IOT_MQTT_Dynamic_Register(MQTTInitParams *pParams);

#ifdef __cplusplus
}
#endif

#endif /* C_SDK_UIOT_EXPORT_MQTT_H_ */
