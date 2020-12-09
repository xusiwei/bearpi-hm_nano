
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"

#include "wifi_device.h"
#include "lwip/netifapi.h"
#include "lwip/api_shell.h"

static int g_connected = 0;

static void PrintLinkedInfo(WifiLinkedInfo* info)
{
    if (!info) return;

    static char macAddress[32] = {0};
    unsigned char* mac = info->bssid;
    snprintf(macAddress, sizeof(macAddress), "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("bssid: %s, rssi: %d, connState: %d, reason: %d, ssid: %s\r\n",
        macAddress, info->rssi, info->connState, info->disconnectedReason, info->ssid);
}


static void OnWifiConnectionChanged(int state, WifiLinkedInfo* info)
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

static void OnWifiScanStateChanged(int state, int size)
{
    printf("%s %d, state = %X, size = %d\r\n", __FUNCTION__, __LINE__, state, size);
}

static void Delay(uint32_t ms)
{
    uint32_t usPerTicks = (1000*1000) / osKernelGetTickFreq();
    osDelay((ms * 1000) / usPerTicks);
    usleep((ms * 1000) % usPerTicks);
}


void WifiConnect(const char *ssid,const char *psk)
{

    WifiErrorCode errCode;
    WifiEvent eventListener = {
        .OnWifiConnectionChanged = OnWifiConnectionChanged,
        .OnWifiScanStateChanged = OnWifiScanStateChanged
    };
    WifiDeviceConfig apConfig = {};
    int netId = -1;

    Delay(10);
    errCode = RegisterWifiEvent(&eventListener);
    printf("RegisterWifiEvent: %d\r\n", errCode);

    // setup your AP params
    strcpy(apConfig.ssid, ssid);
    strcpy(apConfig.preSharedKey, psk);
    apConfig.securityType = WIFI_SEC_TYPE_PSK;


    errCode = EnableWifi();
    printf("EnableWifi: %d\r\n", errCode);
    Delay(100);

    errCode = AddDeviceConfig(&apConfig, &netId);
    printf("AddDeviceConfig: %d\r\n", errCode);

    g_connected = 0;
    errCode = ConnectTo(netId);
    printf("ConnectTo(%d): %d\r\n", netId, errCode);

    while (!g_connected) {
        Delay(10);
    }
    printf("g_connected: %d\r\n", g_connected);
    Delay(3000);

    // 联网业务开始
    // 这里是网络业务代码...
    struct netif* iface = netifapi_netif_find("wlan0");
    if (iface) {
        err_t ret = netifapi_dhcp_start(iface);
        printf("netifapi_dhcp_start: %d\r\n", ret);

        Delay(2000); // wait DHCP server give me IP
        ret = netifapi_netif_common(iface, dhcp_clients_info_show, NULL);
        printf("netifapi_netif_common: %d\r\n", ret);
    }

   
}

