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

#ifndef _ONENET_H_
#define _ONENET_H_


#include <unistd.h>

#include "cJSON.h"

#include "cmsis_os2.h"
#define ONENET_DEBUG                   1

#define ONENET_SW_VERSION              "1.0.0"

#ifndef ONENET_MALLOC
#define ONENET_MALLOC                  malloc
#endif

#ifndef ONENET_CALLOC
#define ONENET_CALLOC                  calloc
#endif

#ifndef ONENET_FREE
#define ONENET_FREE                    free
#endif

#ifndef ONENET_MQTT_SUBTOPIC
#define ONENET_MQTT_SUBTOPIC           "/topic_test"
#endif

#define ONENET_SERVER_URL              "tcp://183.230.40.39:6002"
#define ONENET_SERVER_IP                "183.230.40.39"
#define ONENET_SERVER_PORT              6002
#define ONENET_INFO_DEVID_LEN          16
#define ONENET_INFO_APIKEY_LEN         32
#define ONENET_INFO_PROID_LEN          16
#define ONENET_INFO_AUTH_LEN           64
#define ONENET_INFO_NAME_LEN           64
#define ONENET_INFO_URL_LEN            32

#define ONENET_DATASTREAM_NAME_MAX     32

struct rt_onenet_info
{
    char device_id[ONENET_INFO_DEVID_LEN];
    char api_key[ONENET_INFO_APIKEY_LEN];

    char pro_id[ONENET_INFO_PROID_LEN];
    char auth_info[ONENET_INFO_AUTH_LEN];

    char server_uri[ONENET_INFO_URL_LEN];

    char user_device_id_flg;
};
typedef struct rt_onenet_info *rt_onenet_info_t;

/* onenet datastream info */
struct rt_onenet_ds_info
{
    char id[ONENET_DATASTREAM_NAME_MAX];
    char tags[ONENET_DATASTREAM_NAME_MAX];

    char update_time[ONENET_DATASTREAM_NAME_MAX];
    char create_time[ONENET_DATASTREAM_NAME_MAX];

    char unit[ONENET_DATASTREAM_NAME_MAX];
    char unit_symbol[ONENET_DATASTREAM_NAME_MAX];

    char current_value[ONENET_DATASTREAM_NAME_MAX];

};
typedef struct rt_onenet_ds_info *rt_onenet_ds_info_t;

//设置设备信息
void device_info_init(char *device_id, char * pro_id, char *auth_info, char *api_key, char *master_api_key);

/* OneNET MQTT initialize. */
int onenet_mqtt_init(void);

/* Publish MQTT data to subscribe topic. */
int onenet_mqtt_publish(const char *topic, const uint8_t *msg, size_t len);

#ifdef RT_USING_DFS
/* Publish MQTT binary data to onenet by path. */
int onenet_mqtt_upload_bin_by_path(const char *ds_name, const char *bin_path);
#endif

/* Publish MQTT binary data to onenet. */
int onenet_mqtt_upload_bin(const char *ds_name, uint8_t *bin, size_t len);

/* Publish MQTT string data to onenet. */
int onenet_mqtt_upload_string(const char *ds_name, const char *str);
/* Publish MQTT digit data to onenet. */
int onenet_mqtt_upload_digit(const char *ds_name, const double digit);

/* Device send data to OneNET cloud. */
int onenet_http_upload_digit(const char *ds_name, const double digit);
int onenet_http_upload_string(const char *ds_name, const char *str);

#ifdef ONENET_USING_AUTO_REGISTER
/* Register a device to OneNET cloud. */
int onenet_http_register_device(const char *dev_name, const char *auth_info);
#endif

/* get a datastream from OneNET cloud. */
int onenet_http_get_datastream(const char *ds_name, struct rt_onenet_ds_info *datastream);
/* get datapoints from OneNET cloud. Returned cJSON need to be free when user finished using the data. */
cJSON *onenet_get_dp_by_limit(char *ds_name, size_t limit);
cJSON *onenet_get_dp_by_start_end(char *ds_name, uint32_t start, uint32_t end, size_t limit);
cJSON *onenet_get_dp_by_start_duration(char *ds_name, uint32_t start, size_t duration, size_t limit);
/* Set the command response callback function. User needs to malloc memory for response data. */
void onenet_set_cmd_rsp_cb(void(*cmd_rsp_cb)(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size));

/* ========================== User port function ============================ */
#ifdef ONENET_USING_AUTO_REGISTER
/* Save device info. */
int onenet_port_save_device_info(char *dev_id, char *api_key);
/* Get device name and auth info for register. */
int onenet_port_get_register_info(char *dev_name, char *auth_info);
/* Get device info. */
int onenet_port_get_device_info(char *dev_id, char *api_key, char *auth_info);
/* Check the device has been registered or not. */
int onenet_port_is_registed(void);
#endif

#endif /* _ONENET_H_ */
