## UCloud 软件包API说明

## 可以修改以下宏来调整部分功能

```

/* MQTT心跳消息发送周期, 单位:s */
#define UIOT_MQTT_KEEP_ALIVE_INTERNAL                               (240)

/* MQTT 阻塞调用(包括连接, 订阅, 发布等)的超时时间 */
#define UIOT_MQTT_COMMAND_TIMEOUT                                   (5 * 10000)

/* 接收到 MQTT 包头以后，接收剩余长度及剩余包，最大延迟等待时延 */
#define UIOT_MQTT_MAX_REMAIN_WAIT_MS                                (2000)

/* MQTT消息发送buffer大小, 支持最大256*1024 */
#define UIOT_MQTT_TX_BUF_LEN                                        (2048)

/* MQTT消息接收buffer大小, 支持最大256*1024 */
#define UIOT_MQTT_RX_BUF_LEN                                        (2048)

/* 重连最大等待时间 */
#define MAX_RECONNECT_WAIT_INTERVAL 								(60 * 1000)

/* 使能无限重连，0表示超过重连最大等待时间后放弃重连，
 * 1表示超过重连最大等待时间后以固定间隔尝试重连*/
#define ENABLE_INFINITE_RECONNECT 								    1

```
