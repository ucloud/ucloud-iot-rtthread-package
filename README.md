##  UCloud IOT SDK for rt-thread Package 
## 1 介绍

UCloud IOT SDK for rt-thread Package 是基于[UCloud设备端C-SDK](https://github.com/ucloud/ucloud-iot-device-sdk-c)在RThread环境开发的软件包，用于连接uiot-core物联网平台

### 1.1 SDK架构图
![](https://uiot.cn-sh2.ufileos.com/sdk%E6%9E%B6%E6%9E%84%E5%9B%BE.png)

### 1.2 目录结构

| 名称              | 说明 |
| ----              | ---- |
| uiot              | UCloud设备端C-SDK |
| ports             | 移植文件目录 |
| samples           | 示例目录 |
|  ├─mqtt           | 静态注册收发消息示例 |
|  ├─dynamic_auth   | 动态注册示例 |
|  ├─http           | Http连接云平台示例 |
|  ├─dev_model      | 物模型示例 |
|  ├─ota            | ota升级示例 |
|  ├─shadow         | 设备影子示例 |
| docs              | 说明文档 |
| LICENSE           | 许可证文件 |
| README.md         | 软件包使用说明 |
| SConscript        | RT-Thread 默认的构建脚本 |

### 1.3 许可证

许可协议Apache 2.0。

### 1.4 依赖
Ota功能需要fal软件包
[ ] fal: Flash Abstraction Layer implement. Manage flash device and partition.  --->
Tls功能需要mbedtls软件包
[ ] mbedtls: An portable and flexible SSL/TLS library  ----

## 2 如何打开软件包
menuconfig配置
- RT-Thread env开发工具中使用 `menuconfig` 软件包，配置产品及设备信息，
路径如下：
```
RT-Thread online packages  --->
    IoT - internet of things  --->
        IoT Cloud  --->
            [ ] ucloud_iot_sdk: Ucloud iot sdk for uiot-core platform.  --->
              --- ucloud_iot_sdk:  Ucloud iot sdk for uiot-core platform 
                Ucloud Device Config  --->                  
                [ ]   Enable Mqtt Link uiot-core Platform
                [ ]   Enable Http Link uiot-core Platform
                [ ]   Enable Ucloud Debug
                Version (latest)  --->	
```

## 3 软件包的使用
根据产品需求选择合适的应用示例修改新增业务逻辑，也可新增例程编写新的业务逻辑。
```	
    --- ucloud_iot_sdk: Ucloud iot sdk for uiot-core platform.
          Ucloud Device Config  --->    
    [*]   Enable Mqtt Link uiot-core Platform
    [ ]     Enable Ucloud Mqtt Sample 
    [ ]     Enable Ucloud Mqtt Dynamic Auth Sample
    [*]   Enable Http Link uiot-core Platform
    [ ]     Enable Ucloud Http Publish Sample
    [ ]     Enable Ucloud Http Upload File Sample
    [*]   Enable Shadow      
    [ ]     Enable Ucloud Shadow Sample
    [*]   Enable Dev Model  
    [ ]     Enable Ucloud Dev Model Sample
    [*]   Enable Ota                                                                                                
    [ ]     Enable Ucloud Ota Sample  
    -*-   Enable Tls 
    [ ]   Enable Ucloud Debug
          Version (latest)  --->
```

- 选项说明
`Ucloud Device Config `：填写当前设备认证要素，当认证模式为动态认证时，设备密钥可以不填写

`Enable Mqtt Link uiot-core Platform`：使能MQTT连接云平台功能。

`Enable Ucloud Mqtt Sample`：使能静态注册mqtt和注册成功后收发消息的案例

`Enable Ucloud Mqtt Dynamic Auth Sample`: 使能动态注册mqtt和注册成功后收发消息的案例

`Enable Http Link uiot-core Platform`：使能HTTP连接云平台功能,包括使用https上传消息和上传文件，只支持https,勾选会关联选中mbedTLS软件包。

`Enable Ucloud Http Publish Sample`：使能使用https向云平台上传消息的案例

`Enable Ucloud Http Upload File Sample`: 使能使用https向云平台上传一段buffer在云端保存成文件的案例

`Enable Shadow`：使能设备影子功能

`Enable Ucloud Shadow Sample`：使能设备影子的案例

`Enable Dev Model`：使能物模型功能

`Enable Ucloud Dev Model Sample`：使能物模型的案例

`Enable Ota`：使能远程升级版本的功能，若使能由于要使用flash的分区功能进行ota升级，会关联选中fal软件包。

`Enable Ucloud Ota Sample`：使能远程升级版本的案例

`Enable TLS`： 是否使能TLS，若使能，则会关联选中mbedTLS软件包。

`Enable Ucloud Debug`: 使能调试打印输出

`Version (latest)  --->`：选择软件包的版本

- 使用 `pkgs --update` 命令下载需要使用的软件包

### 2.2 编译及运行
1. 使用命令 scons --target=xxx 输出对应的工程，编译 

2. 打开生成的工程，编译下载到设备中

### 2.4 运行demo程序
系统启动后，在 MSH 中使用命令执行，以mqtt_sample为例：
启动案例：mqtt_test_example start
终止案例：mqtt_test_example stop

执行输出：
msh />mqtt_test_example start                                                                                                       
establish tcp connection with server(host='mqtt-cn-sh2.iot.ucloud.cn', port=[1883])                                                 
msh />success to establish tcp, fd=4                                                                                                
Cloud Device Construct Successsubscribe success, packet-id=2                                                                        
publish success, packet-id=3                                                                                                        
Receive Message With topicName:/fahnimwwvph259ag/3um7h4fdiigf2yyh/upload/event, payload:{"test": "0"}                               
publish success, packet-id=4                                                                                                        
Receive Message With topicName:/fahnimwwvph259ag/3um7h4fdiigf2yyh/upload/event, payload:{"test": "1"}                               
publish success, packet-id=5                                                                                                        
Receive Message With topicName:/fahnimwwvph259ag/3um7h4fdiigf2yyh/upload/event, payload:{"test": "2"}                               
publish success, packet-id=6                                                                                                        
Receive Message With topicName:/fahnimwwvph259ag/3um7h4fdiigf2yyh/upload/event, payload:{"test": "3"}                               
publish success, packet-id=7                                                                                                        
Receive Message With topicName:/fahnimwwvph259ag/3um7h4fdiigf2yyh/upload/event, payload:{"test": "4"}                               
publish success, packet-id=8                                                                                                        
Receive Message With topicName:/fahnimwwvph259ag/3um7h4fdiigf2yyh/upload/event, payload:{"test": "5"}                               
publish success, packet-id=9                                                                                                        
Receive Message With topicName:/fahnimwwvph259ag/3um7h4fdiigf2yyh/upload/event, payload:{"test": "6"}                               
publish success, packet-id=10                                                                                                       
Receive Message With topicName:/fahnimwwvph259ag/3um7h4fdiigf2yyh/upload/event, payload:{"test": "7"}                               
publish success, packet-id=11                                                                                                       
Receive Message With topicName:/fahnimwwvph259ag/3um7h4fdiigf2yyh/upload/event, payload:{"test": "8"}                               
publish success, packet-id=12                                                                                                       
Receive Message With topicName:/fahnimwwvph259ag/3um7h4fdiigf2yyh/upload/event, payload:{"test": "9"} 







