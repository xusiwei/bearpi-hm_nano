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

static char* SecurityTypeName(WifiSecurityType type)
{
    switch (type)
    {
    case WIFI_SEC_TYPE_OPEN:
        return "OPEN";
    case WIFI_SEC_TYPE_WEP:
        return "WEP";
    case WIFI_SEC_TYPE_PSK:
        return "PSK";
    case WIFI_SEC_TYPE_SAE:
        return "SAE";
    default:
        break;
    }
    return "unkow";
}

void PrintScanResult(void)
{
    WifiScanInfo scanResult[WIFI_SCAN_HOTSPOT_LIMIT] = {0};
    uint32_t resultSize = WIFI_SCAN_HOTSPOT_LIMIT;
    /***************获取Wifi热点列表***************/
    memset(&scanResult, 0, sizeof(scanResult));
    WifiErrorCode errCode = GetScanInfoList(scanResult, &resultSize);
    if (errCode != WIFI_SUCCESS) {
        printf("GetScanInfoList failed: %d\r\n", errCode);
    }
    /******串口打印出所有扫描到的Wifi热点信息*******/
    for (uint32_t i = 0; i < resultSize; i++) {
        static char macAddress[32] = {0};
        WifiScanInfo info = scanResult[i];
        unsigned char* mac = info.bssid;
        snprintf(macAddress, sizeof(macAddress), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        printf("Result[%d]: %s, % 4s, %d, %d, %d, %s\r\n", i, macAddress, SecurityTypeName(info.securityType), info.rssi, info.band, info.frequency, info.ssid);
    }
}

static int g_scanDone = 0;
void OnWifiScanStateChangedHandler(int state, int size)
{
    printf("%s %d, state = %X, size = %d\r\n", __FUNCTION__, __LINE__, state, size);

    if (state == WIFI_STATE_AVALIABLE && size > 0) {
        g_scanDone = 1;
    }
}


/**
 * common wait scan result
 */
static void WaitSacnResult(void)
{
    int scanTimeout = 15;
    while (scanTimeout > 0) {
        sleep(1);
        scanTimeout--;
        if (g_scanDone == 1) {
            printf("WaitSacnResult:wait success[%d]s\n", (15 - scanTimeout));
            break;
        }
    }
    if (scanTimeout <= 0) {
        printf("WaitSacnResult:timeout!\n");
    }
}

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

static void WifiScanDemo(void)
{
    osThreadAttr_t attr;

    attr.name = "WifiScanTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 10240;
    attr.priority = 24;

    if (osThreadNew((osThreadFunc_t)WifiScanTask, NULL, &attr) == NULL) {
        printf("Falied to create WifiScanTask!\n");
    }
}

APP_FEATURE_INIT(WifiScanDemo);

