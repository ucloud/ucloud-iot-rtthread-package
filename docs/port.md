## UCloud软件包移植指南

## 移植注意事项

1.需要关注/ports路径下的三个文件夹,目录结构如下：

| 名称            			| 说明 |
| ----            			| ---- |
| fal            			| flash相关 |
|  ├─fal_cfg.h     			| flash分区表配置 |
|  ├─fal_flash_port.c   	| flash驱动 |
| rtthread          		| rtthread系统相关 |
|  ├─HAL_OS_rtthread.c      | 操作系统相关接口 |
|  ├─HAL_TCP_rtthread.c     | 网络操作相关接口 |
|  ├─HAL_Timer_Platform.h   | 定时操作相关接口声明 |
|  ├─HAL_Timer_rtthread.c   | 定时操作相关接口 |
| ssl          				| ssl数据加密相关，ssl功能不开启时可以不关注 |
|  ├─HAL_TLS_config.h       | mbedtls库相关的声明 |
|  ├─HAL_TLS_mbedtls.c      | mbedtls库相关的接口 |

2.移植到新的开发板上时只需要修改fal下的相关的文件，针对使用的开发板flash合理划分分区，修改分区配置表及驱动

3.可以通过修改mbedtls文件夹下的HAL_TLS_config.h打开或关闭宏添加或删除对应的功能
