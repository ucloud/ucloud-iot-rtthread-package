# 编译选项参考

使用本文件的编译选项测试可以通过了除动态注册外的所有sample

## 运行硬件平台

STM32f767ZI + ESP8266

## 硬件平台配置

使用了三个串口，uart2用于和at设备通讯，uart3用于下载调试代码，uart6用于打印输出debug <br>
使用了STM32f767ZI的内置flash <br>

## 开启功能

静态注册模式下，开启了设备影子，物模型和ota升级，没有开启tls

## 依赖软件包

* `SAL套接字抽象层`
* `AT DEVICE`
* `FinSH控制台`
* `netdev网卡抽象层`
* `FAL存储抽象层`

rtthread配置如下：
```     
    #ifndef RT_CONFIG_H__
    #define RT_CONFIG_H__
    
    /* Automatically generated file; DO NOT EDIT. */
    /* RT-Thread Configuration */
    
    /* RT-Thread Kernel */
    
    #define RT_NAME_MAX 8
    #define RT_ALIGN_SIZE 4
    #define RT_THREAD_PRIORITY_32
    #define RT_THREAD_PRIORITY_MAX 32
    #define RT_TICK_PER_SECOND 100
    #define RT_USING_OVERFLOW_CHECK
    #define RT_USING_HOOK
    #define RT_USING_IDLE_HOOK
    #define RT_IDLE_HOOK_LIST_SIZE 4
    #define IDLE_THREAD_STACK_SIZE 1024
    #define RT_USING_TIMER_SOFT
    #define RT_TIMER_THREAD_PRIO 4
    #define RT_TIMER_THREAD_STACK_SIZE 512
    #define RT_DEBUG
    
    /* Inter-Thread communication */
    
    #define RT_USING_SEMAPHORE
    #define RT_USING_MUTEX
    #define RT_USING_EVENT
    #define RT_USING_MAILBOX
    #define RT_USING_MESSAGEQUEUE
    #define RT_USING_SIGNALS
    
    /* Memory Management */
    
    #define RT_USING_MEMPOOL
    #define RT_USING_MEMHEAP
    #define RT_USING_SMALL_MEM
    #define RT_USING_HEAP
    
    /* Kernel Device Object */
    
    #define RT_USING_DEVICE
    #define RT_USING_CONSOLE
    #define RT_CONSOLEBUF_SIZE 1024
    #define RT_CONSOLE_DEVICE_NAME "uart6"
    #define RT_VER_NUM 0x40002
    #define ARCH_ARM
    #define RT_USING_CPU_FFS
    #define ARCH_ARM_CORTEX_M
    #define ARCH_ARM_CORTEX_M7
    
    /* RT-Thread Components */
    
    #define RT_USING_COMPONENTS_INIT
    #define RT_USING_USER_MAIN
    #define RT_MAIN_THREAD_STACK_SIZE 2048
    #define RT_MAIN_THREAD_PRIORITY 10
    
    /* C++ features */
    
    
    /* Command shell */
    
    #define RT_USING_FINSH
    #define FINSH_THREAD_NAME "tshell"
    #define FINSH_USING_HISTORY
    #define FINSH_HISTORY_LINES 5
    #define FINSH_USING_SYMTAB
    #define FINSH_USING_DESCRIPTION
    #define FINSH_THREAD_PRIORITY 20
    #define FINSH_THREAD_STACK_SIZE 4096
    #define FINSH_CMD_SIZE 80
    #define FINSH_USING_MSH
    #define FINSH_USING_MSH_DEFAULT
    #define FINSH_ARG_MAX 10
    
    /* Device virtual file system */
    
    #define RT_USING_DFS
    #define DFS_USING_WORKDIR
    #define DFS_FILESYSTEMS_MAX 2
    #define DFS_FILESYSTEM_TYPES_MAX 2
    #define DFS_FD_MAX 16
    #define RT_USING_DFS_DEVFS
    
    /* Device Drivers */
    
    #define RT_USING_DEVICE_IPC
    #define RT_PIPE_BUFSZ 512
    #define RT_USING_SYSTEM_WORKQUEUE
    #define RT_SYSTEM_WORKQUEUE_STACKSIZE 2048
    #define RT_SYSTEM_WORKQUEUE_PRIORITY 23
    #define RT_USING_SERIAL
    #define RT_SERIAL_USING_DMA
    #define RT_SERIAL_RB_BUFSZ 6144
    #define RT_USING_HWTIMER
    #define RT_USING_PIN
    #define RT_USING_RTC
    #define RT_USING_SOFT_RTC
    
    /* Using USB */
    
    
    /* POSIX layer and C standard library */
    
    #define RT_USING_LIBC
    
    /* Network */
    
    /* Socket abstraction layer */
    
    #define RT_USING_SAL
    
    /* protocol stack implement */
    
    #define SAL_USING_AT
    #define SAL_SOCKETS_NUM 16
    
    /* Network interface device */
    
    #define RT_USING_NETDEV
    #define NETDEV_IPV4 1
    #define NETDEV_IPV6 0
    
    /* light weight TCP/IP stack */
    
    
    /* AT commands */
    
    #define RT_USING_AT
    #define AT_USING_CLIENT
    #define AT_CLIENT_NUM_MAX 1
    #define AT_USING_SOCKET
    #define AT_USING_CLI
    #define AT_CMD_MAX_LEN 1500
    #define AT_SW_VERSION_NUM 0x10300
    
    /* VBUS(Virtual Software BUS) */
    
    
    /* Utilities */
    
    
    /* RT-Thread online packages */
    
    /* IoT - internet of things */
    
    
    /* Wi-Fi */
    
    /* Marvell WiFi */
    
    
    /* Wiced WiFi */
    
    #define PKG_USING_AT_DEVICE
    #define AT_DEVICE_USING_ESP8266
    #define AT_DEVICE_ESP8266_SAMPLE
    #define ESP8266_SAMPLE_WIFI_SSID "wifi_ssid"
    #define ESP8266_SAMPLE_WIFI_PASSWORD "wifi_password"
    #define ESP8266_SAMPLE_CLIENT_NAME "uart2"
    #define ESP8266_SAMPLE_RECV_BUFF_LEN 1500
    #define PKG_USING_AT_DEVICE_LATEST_VERSION
    #define PKG_AT_DEVICE_VER_NUM 0x99999
    
    /* IoT Cloud */
    
    #define PKG_USING_UCLOUD_IOT_SDK
    #define PKG_USING_UCLOUD_MQTT
    #define PKG_USING_UCLOUD_MQTT_STATIC_AUTH
    
    /* Ucloud Device Config */
    
    #define PKG_USING_UCLOUD_IOT_SDK_CONFIG
    #define PKG_USING_UCLOUD_IOT_SDK_PRODUCT_SN "PRODUCT_SN"
    #define PKG_USING_UCLOUD_IOT_SDK_DEVICE_SN "DEVICE_SN"
    #define PKG_USING_UCLOUD_IOT_SDK_DEVICE_SECRET "DEVICE_SECRET"
    #define PKG_USING_UCLOUD_MQTT_SAMPLE
    #define PKG_USING_UCLOUD_SHADOW
    #define PKG_USING_UCLOUD_SHADOW_SAMPLE
    #define PKG_USING_UCLOUD_DEV_MODEL
    #define PKG_USING_UCLOUD_DEV_MODEL_SAMPLE
    #define PKG_USING_UCLOUD_OTA
    #define PKG_USING_UCLOUD_OTA_SAMPLE
    #define PKG_USING_UCLOUD_DEBUG
    #define PKG_USING_UCLOUD_IOT_SDK_LATEST_VERSION
    
    /* security packages */
    
    
    /* language packages */
    
    
    /* multimedia packages */
    
    
    /* tools packages */
    
    
    /* system packages */
    
    #define PKG_USING_FAL
    #define FAL_DEBUG 0
    #define FAL_PART_HAS_TABLE_CFG
    #define PKG_USING_FAL_LATEST_VERSION
    #define PKG_FAL_VER_NUM 0x99999
    
    /* peripheral libraries and drivers */
    
    
    /* miscellaneous packages */
    
    
    /* samples: kernel and components samples */
    
    #define SOC_FAMILY_STM32
    #define SOC_SERIES_STM32F7
    
    /* Hardware Drivers Config */
    
    #define SOC_STM32F767ZI
    
    /* Onboard Peripheral Drivers */
    
    #define BSP_USING_USB_TO_USART
    
    /* On-chip Peripheral Drivers */
    
    #define BSP_USING_GPIO
    #define BSP_USING_UART2
    #define BSP_UART2_RX_USING_DMA
    #define BSP_USING_UART6
    #define BSP_USING_ON_CHIP_FLASH
    #define BSP_USING_UART
    #define BSP_USING_UART3
    
    /* Board extended module Drivers */
    
    
    #endif
```     