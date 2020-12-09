# BearPi-HM_Nano开发板WiFi编程开发——Wifi连接热点


本示例将演示如何在BearPi-HM_Nano开发板上编写一个Wifi连接热点业务程序

![BearPi-HM_Nano](/applications/BearPi/BearPi-HM_Nano/docs/figures/00_public/BearPi-HM_Nano.png)
## Wifi API分析
本案例主要使用了以下几个API完成Wifi联网
## RegisterWifiEvent()
```c
WifiErrorCode RegisterWifiEvent (WifiEvent * event)
```
 **描述：**
为指定的Wi-Fi事件注册回调函数。当WifiEvent中定义的Wi-Fi事件发生时，将调用已注册的回调函数

**参数：**

|名字|描述|
|:--|:------| 
| event | 表示要注册回调的事件.  |


## EnableWifi()
```c
WifiErrorCode EnableWifi (void )
```
**描述：**

启用STA模式

## AddDeviceConfig()
```c
WifiErrorCode AddDeviceConfig (const WifiDeviceConfig * config, int * result )
```
**描述：**

添加用于配置连接到热点信息，此函数生成一个networkId

**参数：**

|名字|描述|
|:--|:------| 
| config | 表示要连接的热点信息.  |
| result | 表示生成的networkId。每个networkId匹配一个热点配置  |

## ConnectTo()
```c
WifiErrorCode ConnectTo (int networkId)
```
**描述：**

连接到指定networkId的热点

**参数：**

|名字|描述|
|:--|:------| 
| networkId | 表示与目标热点匹配的网络id.  |




## netifapi_netif_find()
```c
struct netif *netifapi_netif_find(const char *name);
```
**描述：**

获取netif用于IP操作

## netifapi_netif_find()
```c
struct netif *netifapi_netif_find(const char *name);
```
**描述：**

获取netif用于IP操作

## netifapi_dhcp_start()

```c
err_t netifapi_dhcp_start(n)
```

**描述：**

启动DHCP, 获取IP


## 软件设计

**主要代码分析**

完成Wifi热点的连接需要以下几步

1. 通过 `RegisterWifiEvent` 接口向系统注册扫描状态监听函数，用于接收扫描状态通知，如扫描动作是否完成等
    
* `OnWifiConnectionChangedHandler` 用于绑定连接状态监听函数，该回调函数有两个参数 `state` 和 `info` ；

    * state表示扫描状态，取值为0和1，1表示热点连接成功；

    * info表示Wi-Fi连接信息，包含以下参数；


        |名字|描述|
        |:--|:------| 
        | ssid [WIFI_MAX_SSID_LEN] | 连接的热点名称.  |
        | bssid [WIFI_MAC_LEN] | MAC地址.  |
        | rssi | 接收信号强度(RSSI).  |
        | connState | Wifi连接状态.  |
        | disconnectedReason | Wi-Fi断开的原因.  |




2. 调用 `EnableWifi` 接口，使能 Wifi。
3. 调用 `AddDeviceConfig` 接口，配置连接的热点信息。
4. 调用 `ConnectTo` 接口，连接到指定networkId的热点。
5. 调用 `WaitConnectResult` 接口等待，该函数中会有15s的时间去轮询连接成功标志位 `g_connected`，当`g_connected` 为 1 时退出等待。
6. 调用 `netifapi_netif_find` 接口，获取 netif 用于 IP 操作
7. 调用 `netifapi_dhcp_start` 接口，启动 DHCP, 获取 IP
8. 调用 `netifapi_netif_common` 接口，输出连接信息

```c
static void WifiConnectTask(void)
{
    WifiErrorCode errCode;
    WifiEvent eventListener = {
        .OnWifiConnectionChanged = OnWifiConnectionChangedHandler,
    };
    WifiDeviceConfig apConfig = {0};
    int netId = -1;

    osDelay(10);
    errCode = RegisterWifiEvent(&eventListener);
    printf("RegisterWifiEvent: %d\r\n", errCode);

    // setup your AP params
    strcpy(apConfig.ssid, "TP-LINK_65A8");          //Wifi账号    
    strcpy(apConfig.preSharedKey, "0987654321");    //Wifi密码
    apConfig.securityType = WIFI_SEC_TYPE_PSK;

    while (1) {
        
        errCode = EnableWifi();
        printf("EnableWifi: %d\r\n", errCode);
        osDelay(10);

        errCode = AddDeviceConfig(&apConfig, &netId);
        printf("AddDeviceConfig: %d\r\n", errCode);

        g_connected = 0;
        errCode = ConnectTo(netId);
        printf("ConnectTo(%d): %d\r\n", netId, errCode);

        WaitConnectResult();

        printf("g_connected: %d\r\n", g_connected);
        osDelay(100);

        g_lwip_netif = netifapi_netif_find("wlan0");  // 获取 netif 用于 IP 操作
        if (g_lwip_netif) {
            err_t ret = netifapi_dhcp_start(g_lwip_netif);  //启动 DHCP, 获取 IP
            printf("netifapi_dhcp_start: %d\r\n", ret);

            osDelay(200); // 等待 DHCP 获取IP

            ret = netifapi_netif_common(g_lwip_netif, dhcp_clients_info_show, NULL);  //输出连接信息
            printf("netifapi_netif_common: %d\r\n", ret);
        }
        osDelay(500);

        Disconnect(); // 断开 AP 连接

        RemoveDevice(netId); // 删除连接热点配置参数

        errCode = DisableWifi(); // 关闭 STA 模式
        printf("DisableWifi: %d\r\n", errCode);
        osDelay(500);
    }
}
```

## 编译调试

### 修改 BUILD.gn 文件

修改 `applications\BearPi\BearPi-HM_Nano\sample` 路径下 BUILD.gn 文件，指定 `wifi_connect` 参与编译。
```r
#"D1_iot_wifi_scan:wifi_scan",
"D2_iot_wifi_connect:wifi_connect",        
#"D3_iot_udp_client:udp_client",
#"D4_iot_tcp_server:tcp_server",
#"D5_iot_mqtt:iot_mqtt",        
#"D6_iot_cloud_oc:oc_mqtt",
#"D7_iot_cloud_onenet:onenet_mqtt",
```  
    


### 运行结果<a name="section18115713118"></a>

示例代码编译烧录代码后，按下开发板的RESET按键，通过串口助手查看日志，会打印连接到的Wifi热点信息。
```
=======================================
*********Wifi_Connect_example**********
=======================================
EnableWifi: 0
AddDeviceConfig: 0
ConnectTo(0): 0
+NOTICE:SCANFINISH
+NOTICE:CONNECTED
OnWifiConnectionChangedHandler 33, state = 1, info = 
bssid: 50:FA:84:A5:65:A8, ssid: TP-LINK_65A8, rssi: 0, connState: 0, reason: 0
WaitConnectResult:wait success[1]s
g_connected: 1
netifapi_dhcp_start: 0
server :
        server_id : 192.168.0.1
        mask : 255.255.255.0, 1
        gw : 192.168.0.1
        T0 : 7200
        T1 : 3600
        T2 : 6300
clients <1> :
        mac_idx mac             addr            state   lease   tries   rto     
        0       141131f3032f    192.168.0.116   10      0       1       2       
netifapi_netif_common: 0
+NOTICE:DISCONNECTED
OnWifiConnectionChangedHandler 33, state = 0, info = 
bssid: 50:FA:84:A5:65:A8, ssid: , rssi: 0, connState: 0, reason: 3
DisableWifi: 0
```


