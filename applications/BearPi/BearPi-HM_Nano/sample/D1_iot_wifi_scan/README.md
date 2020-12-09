# BearPi-HM_Nano开发板WiFi编程开发——Wifi热点扫描


本示例将演示如何在BearPi-HM_Nano开发板上编写一个Wifi热点扫描业务程序

![BearPi-HM_Nano](/applications/BearPi/BearPi-HM_Nano/docs/figures/00_public/BearPi-HM_Nano.png)
## Wifi API分析
本案例主要使用了以下几个API完成Wifi热点扫描
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

## Scan()
```c
WifiErrorCode Scan (void )
```
**描述：**

启动Wi-Fi扫描

## GetScanInfoList()
```c
WifiErrorCode GetScanInfoList (WifiScanInfo * result, unsigned int * size )
```
**描述：**

获取在Wi-Fi扫描中检测到的热点列表。Wi-Fi扫描完成后，才能获得热点列表。

**参数：**

|名字|描述|
|:--|:------| 
| result | 表示在Wi-Fi扫描中检测到的热点列表.  |
| size | 表示扫描出的Wifi热点数量  |



## DisableWifi()
```c
WifiErrorCode DisableWifi (void )
```
**描述：**

关闭STA模式



## 软件设计

**主要代码分析**

完成Wifi热点的扫描需要以下几步

1. 通过 `RegisterWifiEvent` 接口向系统注册扫描状态监听函数，用于接收扫描状态通知，如扫描动作是否完成等
    
    * `OnWifiScanStateChangedHandler` 用于绑定扫描状态监听函数，该回调函数有两个参数 `state` 和 `size` ；

        * state表示扫描状态，取值为0和1，1表示扫描动作完成；
        * size表示扫描到的热点个数；
    * 当扫描动作完成后且有扫描到Wifi热点，该回调函数会将扫描成功标志位 `g_scanDone` 置 1 
2. 调用 `EnableWifi` 接口，使能 Wifi STA 模式
3. 调用 `Scan` 接口，触发Wifi扫描动作。
4. 调用 `WaitSacnResult` 接口等待，该函数中会有15s的时间去轮询扫描成功标志位 `g_scanDone`，当`g_scanDone` 为 1 时退出等待。
5. 在 `PrintScanResult` 中调用 `GetScanInfoList` 函数获取扫描结果，GetScanResult函数有两个第二个参数；
    * 第一个参数result指向用于存放结果的数组，需要大于等于WIFI_SCAN_HOTSPOT_LIMIT，
    * 第二个参数size类型为指针是为了内部能够修改它的值，返回后size指向的值是实际搜索到的热点个数；
    * 调用GetScanResult函数前，第二个参数size指向的实际值不能为0，否则会包参数错误；
    
```c
static void WifiScanTask(void)
{
    WifiErrorCode errCode;
    WifiEvent g_wifiEventHandler = {
        .OnWifiScanStateChanged = OnWifiScanStateChangedHandler
    };

    osDelay(10);
    errCode = RegisterWifiEvent(&g_wifiEventHandler);
    printf("RegisterWifiEvent: %d\r\n", errCode);

    while (1) {
        errCode = EnableWifi(); //启用 Wifi STA 模式
        printf("EnableWifi: %d\r\n", errCode);
        osDelay(100);

        g_scanDone = 0;
        errCode = Scan(); //启动Wi-Fi扫描
        printf("Scan: %d\r\n", errCode);

        WaitSacnResult(); //等待扫描结果

        PrintScanResult(); //输出扫描结果

        errCode = DisableWifi(); //关闭 Wifi STA 模式
        printf("DisableWifi: %d\r\n", errCode);
        osDelay(500);
    }
}
```

## 编译调试

### 修改 BUILD.gn 文件

修改 `applications\BearPi\BearPi-HM_Nano\sample` 路径下 BUILD.gn 文件，指定 `wifi_scan` 参与编译。
```r
"D1_iot_wifi_scan:wifi_scan",
#"D2_iot_wifi_connect:wifi_connect",        
#"D3_iot_udp_client:udp_client",
#"D4_iot_tcp_server:tcp_server",
#"D5_iot_mqtt:iot_mqtt",        
#"D6_iot_cloud_oc:oc_mqtt",
#"D7_iot_cloud_onenet:onenet_mqtt",
```    


### 运行结果<a name="section18115713118"></a>

示例代码编译烧录代码后，按下开发板的RESET按键，通过串口助手查看日志，会打印扫描到的Wifi热点信息。
```
RegisterWifiEvent: 0
EnableWifi: 0
OnWifiScanStateChangedHandler 56, state = 0, size = 0
Scan: 0
+NOTICE:SCANFINISH
OnWifiScanStateChangedHandler 56, state = 1, size = 11
WaitSacnResult:wait success[1]s
Result[0]: 4C:D1:A1:54:5F:BC,  PSK, -4100, 0, 2462, ChinaNet-FFCV95
Result[1]: 4C:D1:A1:54:5F:C1, unkow, -4200, 0, 2462, 
Result[2]: 4C:D1:A1:54:5F:BD, unkow, -4500, 0, 2462, 
Result[3]: 68:D1:BA:EA:65:1A,  PSK, -5700, 0, 2437, CMCC-SRQH
Result[4]: 50:FA:84:A5:65:A8,  PSK, -6200, 0, 2462, TP-LINK_65A8
Result[5]: B0:CC:FE:67:23:9C, unkow, -8000, 0, 2412, 807
Result[6]: B0:CC:FE:67:23:A1, unkow, -8000, 0, 2412, 
Result[7]: F4:83:CD:B6:37:8C,  PSK, -8900, 0, 2412, fuguang2.4g
Result[8]: EC:56:23:03:E1:F1, unkow, -9000, 0, 2437, 
Result[9]: 54:75:95:F1:FF:45,  PSK, -9100, 0, 2467, TP-LINK_ISU_2019
Result[10]: D0:76:E7:59:7B:28,  PSK, -9200, 0, 2462, 1406
DisableWifi: 0
```


