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


#ifndef C_SDK_MQTT_CLIENT_H_
#define C_SDK_MQTT_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "uiot_export.h"
#include "uiot_import.h"
#include "uiot_internal.h"

#include "mqtt_client_net.h"

#include "utils_timer.h"
#include "utils_list.h"

/* 报文id最大值 */
#define MAX_PACKET_ID                                               (65535)

/* 成功订阅主题的最大个数 */
#define MAX_SUB_TOPICS                                              (10)

/* 在列表中最大的重新发布数量 */
#define MAX_REPUB_NUM                                               (20)

/* 重连最小等待时间 */
#define MIN_RECONNECT_WAIT_INTERVAL                                 (1000)

/* MQTT报文最小超时时间 */
#define MIN_COMMAND_TIMEOUT                                         (500)

/* MQTT报文最大超时时间 */
#define MAX_COMMAND_TIMEOUT                                         (5000)

/* 云端保留主题的最大长度 */
#define MAX_SIZE_OF_CLOUD_TOPIC                                     (128)

/* 动态注册相关的topic */
#define DYNAMIC_REGISTER_PUB_TEMPLATE          "/$system/%s/%s/password"

#define DYNAMIC_REGISTER_SUB_TEMPLATE          "/$system/%s/%s/password_reply"

/* 获取设备密钥的消息中的字段 */
#define PASSWORD_REPLY_RETCODE                 "RetCode"

#define PASSWORD_REPLY_REQUEST_ID              "RequestID"

#define PASSWORD_REPLY_PASSWORD                "Password"

/**
 * @brief MQTT Message Type
 */
typedef enum msgTypes {
    RESERVED    = 0,     // Reserved
    CONNECT     = 1,     // Client request to connect to Server
    CONNACK     = 2,     // Connect Acknowledgment
    PUBLISH     = 3,     // Publish message
    PUBACK      = 4,     // Publish Acknowledgment
    PUBREC      = 5,     // Publish Received
    PUBREL      = 6,     // Publish Release
    PUBCOMP     = 7,     // Publish Complete
    SUBSCRIBE   = 8,     // Client Subscribe request
    SUBACK      = 9,     // Subscribe Acknowledgment
    UNSUBSCRIBE = 10,    // Client Unsubscribe request
    UNSUBACK    = 11,    // Unsubscribe Acknowledgment
    PINGREQ     = 12,    // PING Request
    PINGRESP    = 13,    // PING Response
    DISCONNECT  = 14     // Client is Disconnecting
} MessageTypes;

typedef enum {
    DISCONNECTED = 0,
    CONNECTED    = 1
} ConnStatus;

typedef enum {
    STATIC_AUTH  = 1,        
    DYNAMIC_AUTH = 2,
} AuthStatus;

/** 
 * MQTT byte 1: fixed header
 * bits  |7654: Message Type  | 3:DUP flag |  21:QoS level | 0:RETAIN |
 */
#define MQTT_HEADER_TYPE_SHIFT          0x04
#define MQTT_HEADER_TYPE_MASK           0xF0
#define MQTT_HEADER_DUP_SHIFT           0x03
#define MQTT_HEADER_DUP_MASK            0x08
#define MQTT_HEADER_QOS_SHIFT           0x01
#define MQTT_HEADER_QOS_MASK            0x06
#define MQTT_HEADER_RETAIN_MASK         0x01


/**
 * @brief MQTT 遗嘱消息参数结构体定义
 *
 * 当客户端异常断开连接, 若客户端在连接时有设置遗嘱消息, 服务端会发布遗嘱消息。
 */
typedef struct {
    char        struct_id[4];      // The eyecatcher for this structure. must be MQTW
    uint8_t     struct_version;    // 结构体 0
    char        *topic_name;       // 遗嘱消息主题名
    char        *message;          // 遗嘱消息负载部分数据
    uint8_t     retained;          // 遗嘱消息retain标志位
    QoS         qos;               // 遗嘱消息qos标志位
} WillOptions;

/**
 * MQTT遗嘱消息结构体默认值定义
 */
#define DEFAULT_WILL_OPTIONS { {'M', 'Q', 'T', 'W'}, 0, NULL, NULL, 0, QOS0 }

/**
 * @brief MQTT 连接参数结构体定义
 *
 */
typedef struct {
    char                        *client_id;             // 客户端标识符, 请保持唯一
    char                        *username;              // 用户名
    char                        *password;                // 密码

    char                        struct_id[4];           // The eyecatcher for this structure.  must be MQTC.
    uint8_t                     struct_version;         // 结构体版本号, 必须为0
    uint8_t                     mqtt_version;           // MQTT版本协议号 4 = 3.1.1
    uint16_t                    keep_alive_interval;    // 心跳周期, 单位: s
    uint8_t                     clean_session;          // 清理会话标志位, 具体含义请参考MQTT协议说明文档3.1.2.4小结

    uint8_t                     auto_connect_enable;    // 是否开启自动重连
} MQTTConnectParams;

/**
 * MQTT连接参数结构体默认值定义
 */
#define DEFAULT_MQTT_CONNECT_PARAMS { NULL, NULL, NULL, {'M', 'Q', 'T', 'C'}, 0, 4, 240, 1 , 1,}

/**
 * @brief 订阅主题对应的消息处理结构体定义
 */
typedef struct SubTopicHandle {
    const char              *topic_filter;               // 订阅主题名, 可包含通配符
    OnMessageHandler        message_handler;             // 订阅主题消息回调:wq函数指针
    void                    *message_handler_data;       // 用户数据, 通过回调函数返回
    QoS                     qos;                         // 服务质量等级
} SubTopicHandle;

/**
 * @brief MQTT Client结构体定义
 */
typedef struct Client {
    uint8_t                  is_connected;                                  // 网络是否连接
    uint8_t                  was_manually_disconnected;                     // 是否手动断开连接
    uint8_t                  is_ping_outstanding;                           // 心跳包是否未完成, 即未收到服务器响应

    uint16_t                 next_packet_id;                                // MQTT报文标识符
    uint32_t                 command_timeout_ms;                            // MQTT消息超时时间, 单位:ms

    uint32_t                 current_reconnect_wait_interval;               // MQTT重连周期, 单位:ms
    uint32_t                 counter_network_disconnected;                  // 网络断开连接次数

    size_t                   write_buf_size;                                // 消息发送buffer长度
    size_t                   read_buf_size;                                 // 消息接收buffer长度
    unsigned char            write_buf[UIOT_MQTT_TX_BUF_LEN];               // MQTT消息发送buffer
    unsigned char            read_buf[UIOT_MQTT_RX_BUF_LEN];                // MQTT消息接收buffer

    void                     *lock_generic;                                 // client原子锁
    void                     *lock_write_buf;                               // 输出流的锁

    void                     *lock_list_pub;                                // 等待发布消息ack列表的锁
    void                     *lock_list_sub;                                // 等待订阅消息ack列表的锁

    List                     *list_pub_wait_ack;                            // 等待发布消息ack列表
    List                     *list_sub_wait_ack;                            // 等待订阅消息ack列表

    MQTTEventHandler         event_handler;                                 // 事件句柄

    MQTTConnectParams        options;                                       // 连接相关参数

    utils_network_t          network_stack;                                 // MQTT底层使用的网络参数

    Timer                    ping_timer;                                    // MQTT心跳包发送定时器
    Timer                    reconnect_delay_timer;                         // MQTT重连定时器, 判断是否已到重连时间

    SubTopicHandle           sub_handles[MAX_SUB_TOPICS];                   // 订阅主题对应的消息处理结构数组
} UIoT_Client;

/**
 * @brief MQTT协议版本
 */
typedef enum {
    MQTT_3_1_1 = 4
} MQTT_VERSION;


typedef enum MQTT_NODE_STATE {
    MQTT_NODE_STATE_NORMAL = 0,
    MQTT_NODE_STATE_INVALID,
} MQTTNodeState;

/* 记录已经发布的topic的信息 */
typedef struct REPUBLISH_INFO {
    Timer                   pub_start_time;     /* 发布的时间 */
    MQTTNodeState           node_state;         /* 节点状态 */
    uint16_t                msg_id;             /* 发布消息的packet id */
    uint32_t                len;                /* 消息长度 */
    unsigned char           *buf;               /* 消息内容 */
} UIoTPubInfo;

/* 记录已经订阅的topic的信息 */
typedef struct SUBSCRIBE_INFO {
    enum msgTypes           type;           /* 类型, (sub or unsub) */
    uint16_t                msg_id;         /* 订阅/取消订阅的 packet id */
    Timer                   sub_start_time; /* 订阅的开始时间 */
    MQTTNodeState           node_state;     /* 节点状态 */
    SubTopicHandle          handler;        /* handle of topic subscribed(unsubscribed) */
    uint16_t                len;            /* 消息长度 */
    unsigned char           *buf;           /* 消息内容 */
} UIoTSubInfo;

/**
 * @brief 对结构体Client进行初始化
 *
 * @param pClient MQTT客户端结构体
 * @param pParams MQTT客户端初始化参数
 * @return 返回SUCCESS, 表示成功
 */
int uiot_mqtt_init(UIoT_Client *pClient, MQTTInitParams *pParams);

/**
 * @brief 建立基于TLS的MQTT连接
 *
 * 注意: Client ID不能为NULL或空字符串
 *
 * @param pClient MQTT客户端结构体
 * @param pParams MQTT连接相关参数, 可参考MQTT协议说明
 * @return 返回SUCCESS, 表示成功
 */
int uiot_mqtt_connect(UIoT_Client *pClient, MQTTConnectParams *pParams);

/**
 * @brief 与服务器重新建立MQTT连接
 *
 * 1. 与服务器重新建立MQTT连接
 * 2. 连接成功后, 重新订阅之前的订阅过的主题
 *
 * @param pClient MQTT Client结构体
 *
 * @return 返回ERR_MQTT_RECONNECTED, 表示重连成功
 */
int uiot_mqtt_attempt_reconnect(UIoT_Client *pClient);

/**
 * @brief 断开MQTT连接
 *
 * @param pClient MQTT Client结构体
 * @return 返回SUCCESS, 表示成功
 */
int uiot_mqtt_disconnect(UIoT_Client *pClient);

/**
 * @brief 发布MQTT消息
 *
 * @param pClient MQTT客户端结构体
 * @param topicName 主题名
 * @param pParams 发布参数
 * @return < 0  :   表示失败
 *         >= 0 :   返回唯一的packet id 
 */
int uiot_mqtt_publish(UIoT_Client *pClient, char *topicName, PublishParams *pParams);

/**
 * @brief 订阅MQTT主题
 *
 * @param pClient MQTT客户端结构体
 * @param topicFilter 主题过滤器, 可参考MQTT协议说明 4.7
 * @param pParams 订阅参数
 * @return < 0  :   表示失败
 *         >= 0 :   返回唯一的packet id 
 */
int uiot_mqtt_subscribe(UIoT_Client *pClient, char *topicFilter, SubscribeParams *pParams);

/**
 * @brief 重新订阅断开连接之前已订阅的主题
 *
 * @param pClient MQTT客户端结构体
 * @return 返回SUCCESS, 表示成功
 */
int uiot_mqtt_resubscribe(UIoT_Client *pClient);

/**
 * @brief 取消订阅已订阅的MQTT主题
 *
 * @param pClient MQTT客户端结构体
 * @param topicFilter  主题过滤器, 可参考MQTT协议说明 4.7
 * @return < 0  :   表示失败
 *         >= 0 :   返回唯一的packet id   
 */
int uiot_mqtt_unsubscribe(UIoT_Client *pClient, char *topicFilter);

/**
 * @brief 在当前线程为底层MQTT客户端让出一定CPU执行时间
 *
 * 在这段时间内, MQTT客户端会用用处理消息接收, 以及发送PING报文, 监控网络状态
 *
 * @param pClient    MQTT Client结构体
 * @param timeout_ms Yield操作超时时间
 * @return 返回SUCCESS, 表示成功, 返回ERR_MQTT_ATTEMPTING_RECONNECT, 表示正在重连
 */
int uiot_mqtt_yield(UIoT_Client *pClient, uint32_t timeout_ms);

/**
 * @brief 客户端自动重连是否开启
 *
 * @param pClient MQTT客户端结构体
 * @return 返回true, 表示客户端已开启自动重连功能
 */
bool uiot_mqtt_is_autoreconnect_enabled(UIoT_Client *pClient);

/**
 * @brief 设置客户端是否开启自动重连
 *
 * @param pClient MQTT客户端结构体
 * @param value 是否开启该功能
 * @return 返回SUCCESS, 表示设置成功
 */
int uiot_mqtt_set_autoreconnect(UIoT_Client *pClient, bool value);

/**
 * @brief 获取网络断开的次数
 *
 * @param pClient MQTT Client结构体
 * @return 返回客户端MQTT网络断开的次数
 */
int uiot_mqtt_get_network_disconnected_count(UIoT_Client *pClient);

/**
 * @brief 重置连接断开次数
 *
 * @param pClient MQTT Client结构体
 * @return 返回SUCCESS, 表示设置成功
 */
int uiot_mqtt_reset_network_disconnected_count(UIoT_Client *pClient);

/**
 * @brief 获取报文标识符
 *
 * @param pClient
 * @return
 */
uint16_t get_next_packet_id(UIoT_Client *pClient);

/**
 *
 * @param header
 * @param message_type
 * @param qos
 * @param dup
 * @param retained
 * @return
 */
int mqtt_init_packet_header(unsigned char *header, MessageTypes message_type, QoS qos, uint8_t dup, uint8_t retained);

/**
 * @brief 接收服务端消息
 *
 * @param pClient
 * @param timer
 * @param packet_type
 * @param qos
 * @return
 */
int cycle_for_read(UIoT_Client *pClient, Timer *timer, uint8_t *packet_type, QoS qos);

/**
 * @brief 发送报文数据
 *
 * @param pClient       Client结构体
 * @param length        报文长度
 * @param timer         定时器
 * @return
 */
int send_mqtt_packet(UIoT_Client *pClient, size_t length, Timer *timer);

/**
 * @brief 等待指定类型的MQTT控制报文
 *
 * only used in single-threaded mode where one command at a time is in process
 *
 * @param pClient       MQTT Client结构体
 * @param packet_type   控制报文类型
 * @param timer         定时器
 * @return
 */
int wait_for_read(UIoT_Client *pClient, uint8_t packet_type, Timer *timer, QoS qos);

/**
 * @brief 设置MQTT当前连接状态
 *
 * @param pClient       Client结构体
 * @param connected     0: 连接断开状态  1: 已连接状态
 * @return
 */
void set_client_conn_state(UIoT_Client *pClient, uint8_t connected);

/**
 * @brief 获取MQTT当前连接状态
 *
 * @param pClient         Client结构体
 * @return                0: 连接断开状态  1: 已连接状态
 */
uint8_t get_client_conn_state(UIoT_Client *pClient);

/**
 * @brief 检查 Publish ACK 等待列表，若有成功接收或者超时，则将对应节点从列表中移除
 *
 * @param pClient MQTT客户端
 * @return 返回SUCCESS, 表示成功
 */
int uiot_mqtt_pub_info_proc(UIoT_Client *pClient);

/**
 * @brief 检查 Subscribe ACK 等待列表，若有成功接收或者超时，则将对应节点从列表中移除
 *
 * @param pClient MQTT客户端
 * @return 返回SUCCESS, 表示成功
 */
int uiot_mqtt_sub_info_proc(UIoT_Client *pClient);

int push_sub_info_to(UIoT_Client *c, int len, unsigned short msgId, MessageTypes type,
                                   SubTopicHandle *handler, ListNode **node);

int serialize_pub_ack_packet(unsigned char *buf, size_t buf_len, MessageTypes packet_type, uint8_t dup,
                             uint16_t packet_id,
                             uint32_t *serialized_len);

int serialize_packet_with_zero_payload(unsigned char *buf, size_t buf_len, MessageTypes packetType, uint32_t *serialized_len);

int deserialize_publish_packet(unsigned char *dup, QoS *qos, uint8_t *retained, uint16_t *packet_id, char **topicName,
                               uint16_t *topicNameLen, unsigned char **payload, size_t *payload_len, unsigned char *buf, size_t buf_len);

int deserialize_suback_packet(uint16_t *packet_id, uint32_t max_count, uint32_t *count,
                                     QoS *grantedQoSs, unsigned char *buf, size_t buf_len);

int deserialize_unsuback_packet(uint16_t *packet_id, unsigned char *buf, size_t buf_len);

int deserialize_ack_packet(uint8_t *packet_type, uint8_t *dup, uint16_t *packet_id, unsigned char *buf, size_t buf_len);

bool parse_mqtt_payload_retcode_type(char *pJsonDoc, uint32_t *pRetCode); 

bool parse_mqtt_state_request_id_type(char *pJsonDoc, char **pType);

bool parse_mqtt_state_password_type(char *pJsonDoc, char **pType);

#ifdef MQTT_CHECK_REPEAT_MSG

void reset_repeat_packet_id_buffer(void);

#endif
/**
 * @brief 根据剩余长度计算整个MQTT报文的长度
 *
 * @param rem_len    剩余长度
 * @return           整个MQTT报文的长度
 */
size_t get_mqtt_packet_len(size_t rem_len);

size_t mqtt_write_packet_rem_len(unsigned char *buf, uint32_t length);

int mqtt_read_packet_rem_len_form_buf(unsigned char *buf, uint32_t *value, uint32_t *readBytesLen);

uint16_t mqtt_read_uint16_t(unsigned char **pptr);

unsigned char mqtt_read_char(unsigned char **pptr);

void mqtt_write_char(unsigned char **pptr, unsigned char c);

void mqtt_write_uint_16(unsigned char **pptr, uint16_t anInt);

void mqtt_write_utf8_string(unsigned char **pptr, const char *string);

#ifdef __cplusplus
}
#endif

#endif //C_SDK_MQTT_CLIENT_H_

