# 快速入门示例
为了快速的让您了解如何将您的设备连接到物联网平台，我们通过一个设备向拥有订阅&发布权限的topic发送消息，并接收返回的消息作为示例讲解。

## 操作步骤

### 创建产品与设备
将设备接入物联网平台的第一步是在平台创建产品。产品相当于某一类设备的集合，该类设备具有相同的功能，您可以根据产品批量管理对应设备。

1. [注册](https://passport.ucloud.cn/#register)UCloud云服务
2. 登录进入UCloud[物联网平台](https://console.ucloud.cn/uiot)
3. 创建产品  

   - 点击<创建产品>，创建一个产品，为其命名，点击<确定>；
   - 创建完成后，点击产品的详情，可以对产品进行相应的配置。
   
   ![添加产品](/images/添加产品.png)

4. 创建设备

   - 添加设备，点击<设备管理>、<添加设备>、<随机生成>、<生成设备个数1个>、<确定>； 
   
   ![随机添加设备](/images/随机添加设备.png)


5. 记录设备注册信息  
 
   - 点击添加的设备，打开设备详情页，准备好设备注册相关信息，设备注册信息包含：`产品序列号` `设备序列号` `设备密码`，需要妥善保管好，后续测试需要使用。  
   
   ![设备注册信息](/images/设备注册信息.png)


### 建立设备与平台的连接

这里使用基于rtthread操作系统的软件包，快速将设备接入到物联网平台。

在C-SDK的目录`sample/mqtt/`中，通过修改例程`mqtt_sample.c`来介绍如何使用C-SDK。


#### 配置设备信息
通过rtthread的env配置工具配置产品和设备信息。
    
1.  打开mqtt sample
```            
    --- ucloud-iot-sdk: Ucloud iot sdk for uiot-core platform.
          Ucloud Device Config  --->    	
    [*]   Enable Mqtt Link uiot-core Platform
    [*]     Enable Ucloud Mqtt Sample 
    [ ]     Enable Ucloud Mqtt Dynamic Auth Sample
    [ ]   Enable Http Link uiot-core Platform
    [ ]   Enable Shadow      
    [ ]   Enable Dev Model  
    [ ]   Enable Ota                                                                                                
    [ ]   Enable Tls 
    [ ]   Enable Ucloud Debug
          Version (latest)  --->
```

2.配置产品设备认证三要素
```
    Ucloud Device Config    --->
    [*] Device Config  ---- 
    (5xaptnq5is1xt45c) Config Product SN
    (dy0jndfndj6wvvq0) Config Product Secret
    (tycfyk7697ra5jqs) Config Device SN                                                                           
    (6kh48wf6oq5xktir) Config Device Secret
```

3. 保存配置

4. 编译生成版本，下载到开发板中，系统启动后，在 MSH 中使用命令执行，
```
msh />mqtt_test_example start 
```

5. 查看日志

平台提供日志管理功能可以查看设备行为以及所有经过平台流转的上行或下行的数据。
    
1.设备上下线

   ![设备上线](/images/设备上线.png)
    
2.设备上行消息

   ![设备上行消息](/images/设备上行消息.png)
    
3.设备下行消息

   ![设备下行消息](/images/设备下行消息.png)
