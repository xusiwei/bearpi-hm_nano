/*
 * Copyright (c) 2020 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifi_device.h"

#include "lwip/netifapi.h"
#include "lwip/api_shell.h"

static struct netif *g_lwip_netif = NULL;


static void PrintLinkedInfo(WifiLinkedInfo* info)
{
    if (!info) return;

    static char macAddress[32] = {0};
    unsigned char* mac = info->bssid;
    snprintf(macAddress, sizeof(macAddress), "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("bssid: %s, ssid: %s, rssi: %d, connState: %d, reason: %d\r\n",
        macAddress, info->ssid, info->rssi, info->connState, info->disconnectedReason);
}

static int g_connected = 0;
static void OnWifiConnectionChangedHandler(int state, WifiLinkedInfo* info)
{
    if (!info) return;

    printf("%s %d, state = %d, info = \r\n", __FUNCTION__, __LINE__, state);
    PrintLinkedInfo(info);

    if (state == WIFI_STATE_AVALIABLE) {
        g_connected = 1;
    } else {
        g_connected = 0;
    }
}

/**
 * common wait Connect result
 */
static void WaitConnectResult(void)
{
    int ConnectTimeout = 15;
    while (ConnectTimeout > 0) {
        sleep(1);
        ConnectTimeout--;
        if (g_connected == 1) {
            printf("WaitConnectResult:wait success[%d]s\n", (15 - ConnectTimeout));
            break;
        }
    }
    if (ConnectTimeout <= 0) {
        printf("WaitConnectResult:timeout!\n");
    }
}

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

    while (1) 
    {
        printf("=======================================\r\n");
        printf("*********Wifi_Connect_example**********\r\n");
        printf("=======================================\r\n");
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

static void WifiConnectDemo(void)
{
    osThreadAttr_t attr;

    attr.name = "WifiConnectTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 10240;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)WifiConnectTask, NULL, &attr) == NULL) {
        printf("Falied to create WifiConnectTask!\n");
    }
}

SYS_RUN(WifiConnectDemo);
