/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Sergio R. Caprile - clarifications and/or documentation extension
 *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"

#include "wifi_connect.h"
#include "lwip/sockets.h"

#include "oc_mqtt.h"
#include "E53_IA1.h"


#define MSGQUEUE_OBJECTS 16                     // number of Message Queue Objects
 
typedef struct {                                // object data type
  char *Buf;
  uint8_t Idx;
} MSGQUEUE_OBJ_t;
 
MSGQUEUE_OBJ_t msg;
osMessageQueueId_t mid_MsgQueue;                // message queue id

#define CLIENT_ID "5fc7152fb4ec2202e99df7bf_2020111409_0_0_2020120205"
#define USERNAME "5fc7152fb4ec2202e99df7bf_2020111409"
#define PASSWORD "9b501c0ce96e0d79a071703b870060d939e95715397c6cb5dee90a93a38dd288"


typedef enum
{
    en_msg_cmd = 0,
    en_msg_report,
}en_msg_type_t;

typedef struct
{
    char *request_id;
    char *payload;
}cmd_t;

typedef struct
{
    int lum;
    int temp;
    int hum;
}report_t;

typedef struct
{
    en_msg_type_t msg_type;
    union{
        cmd_t cmd;
        report_t report;
    }msg;
}app_msg_t;

typedef struct
{
    int                          connected;
    int                          led;
    int                          motor;
}app_cb_t;
static app_cb_t  g_app_cb;

static void deal_report_msg(report_t *report)
{
    oc_mqtt_profile_service_t    service;
    oc_mqtt_profile_kv_t         temperature;
    oc_mqtt_profile_kv_t         humidity;
    oc_mqtt_profile_kv_t         luminance;
    oc_mqtt_profile_kv_t         led;
    oc_mqtt_profile_kv_t         motor;


    service.event_time = NULL;
    service.service_id = "Agriculture";
    service.service_property = &temperature;
    service.nxt = NULL;

    temperature.key = "Temperature";
    temperature.value = &report->temp;
    temperature.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    temperature.nxt = &humidity;

    humidity.key = "Humidity";
    humidity.value = &report->hum;
    humidity.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    humidity.nxt = &luminance;

    luminance.key = "Luminance";
    luminance.value = &report->lum;
    luminance.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    luminance.nxt = &led;

    led.key = "LightStatus";
    led.value = g_app_cb.led?"ON":"OFF";
    led.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    led.nxt = &motor;

    motor.key = "MotorStatus";
    motor.value = g_app_cb.motor?"ON":"OFF";
    motor.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    motor.nxt = NULL;

    oc_mqtt_profile_propertyreport(USERNAME,&service);
    return;
}

void oc_cmd_rsp_cb(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size)
{
	app_msg_t *app_msg;

	int    ret = 0;
	app_msg = malloc(sizeof(app_msg_t));
	app_msg->msg_type = en_msg_cmd;
	app_msg->msg.cmd.payload = (char *)recv_data;

    printf("recv data is %.*s\n", recv_size, recv_data);
    ret = osMessageQueuePut(mid_MsgQueue,&app_msg,0U, 0U);
    if(ret != 0){
        free(recv_data);
    }
    *resp_data = NULL;
    *resp_size = 0;
}

///< COMMAND DEAL
#include <cJSON.h>
static void deal_cmd_msg(cmd_t *cmd)
{
    cJSON *obj_root;
    cJSON *obj_cmdname;
    cJSON *obj_paras;
    cJSON *obj_para;

    int cmdret = 1;
    oc_mqtt_profile_cmdresp_t  cmdresp;
    obj_root = cJSON_Parse(cmd->payload);
    if(NULL == obj_root){
        goto EXIT_JSONPARSE;
    }

    obj_cmdname = cJSON_GetObjectItem(obj_root,"command_name");
    if(NULL == obj_cmdname){
        goto EXIT_CMDOBJ;
    }
    if(0 == strcmp(cJSON_GetStringValue(obj_cmdname),"Agriculture_Control_light")){
        obj_paras = cJSON_GetObjectItem(obj_root,"paras");
        if(NULL == obj_paras){
            goto EXIT_OBJPARAS;
        }
        obj_para = cJSON_GetObjectItem(obj_paras,"Light");
        if(NULL == obj_para){
            goto EXIT_OBJPARA;
        }
        ///< operate the LED here
        if(0 == strcmp(cJSON_GetStringValue(obj_para),"ON")){
            g_app_cb.led = 1;
            Light_StatusSet(ON);
            printf("Light On!");
        }
        else{
            g_app_cb.led = 0;
            Light_StatusSet(OFF);
            printf("Light Off!");
        }
        cmdret = 0;
    }
    else if(0 == strcmp(cJSON_GetStringValue(obj_cmdname),"Agriculture_Control_Motor")){
        obj_paras = cJSON_GetObjectItem(obj_root,"Paras");
        if(NULL == obj_paras){
            goto EXIT_OBJPARAS;
        }
        obj_para = cJSON_GetObjectItem(obj_paras,"Motor");
        if(NULL == obj_para){
            goto EXIT_OBJPARA;
        }
        ///< operate the Motor here
        if(0 == strcmp(cJSON_GetStringValue(obj_para),"ON")){
            g_app_cb.motor = 1;
            Motor_StatusSet(ON);
            printf("Motor On!");
        }
        else{
            g_app_cb.motor = 0;
            Motor_StatusSet(OFF);
            printf("Motor Off!");
        }
        cmdret = 0;
    }

EXIT_OBJPARA:
EXIT_OBJPARAS:
EXIT_CMDOBJ:
    cJSON_Delete(obj_root);
EXIT_JSONPARSE:
    ///< do the response
    cmdresp.paras = NULL;
    cmdresp.request_id = cmd->request_id;
    cmdresp.ret_code = cmdret;
    cmdresp.ret_name = NULL;
    (void)oc_mqtt_profile_cmdresp(NULL,&cmdresp);
    return;
}


static int task_main_entry( void )
{
    app_msg_t *app_msg;

	WifiConnect("TP-LINK_65A8","0987654321");
	device_info_init(CLIENT_ID,USERNAME,PASSWORD);
	oc_mqtt_init();
	oc_set_cmd_rsp_cb(oc_cmd_rsp_cb);

    while(1){
        app_msg = NULL;
        (void)osMessageQueueGet(mid_MsgQueue,(void **)&app_msg,NULL, 0U);
        if(NULL != app_msg){
            switch(app_msg->msg_type){
                case en_msg_cmd:
                    deal_cmd_msg(&app_msg->msg.cmd);
                    break;
                case en_msg_report:
                    deal_report_msg(&app_msg->msg.report);
                    break;
                default:
                    break;
            }
            free(app_msg);
        }
    }
    return 0;
}

static int task_sensor_entry( void)
{
    app_msg_t *app_msg;
    E53_IA1_Data_TypeDef  data;
	E53_IA1_Init();
    while(1){
        E53_IA1_Read_Data(&data);
        app_msg = malloc(sizeof(app_msg_t));
        printf("SENSOR:lum:%.2f temp:%.2f hum:%.2f\r\n",data.Lux, data.Temperature,data.Humidity);
        if(NULL != app_msg){
            app_msg->msg_type = en_msg_report;
            app_msg->msg.report.hum = (int)data.Humidity;
            app_msg->msg.report.lum = (int)data.Lux;
            app_msg->msg.report.temp = (int)data.Temperature;
            if(0 != osMessageQueuePut(mid_MsgQueue,&app_msg,0U, 0U)){
                free(app_msg);
            }
        }
        sleep(3);
    }
    return 0;
}

static void MQTT_Demo(void)
{
	mid_MsgQueue = osMessageQueueNew(MSGQUEUE_OBJECTS, 10, NULL);
	if (mid_MsgQueue == NULL) {
	printf("Falied to create Message Queue!\n");
	}
	
	osThreadAttr_t attr;

    attr.name = "task_main_entry";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 10240;
    attr.priority = 24;

    if (osThreadNew((osThreadFunc_t)task_main_entry, NULL, &attr) == NULL) {
        printf("Falied to create task_main_entry!\n");
    }
	attr.stack_size = 2048;
    attr.priority = 25;
	attr.name = "task_sensor_entry";
	if (osThreadNew((osThreadFunc_t)task_sensor_entry, NULL, &attr) == NULL) {
		printf("Falied to create task_sensor_entry!\n");
	}
}

APP_FEATURE_INIT(MQTT_Demo);