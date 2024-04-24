/*
*********************************************************************************************************
*
*	模块名称 : 启动任务
*	文件名称 : hello_task.h
*	版    本 : V1.0
*	说    明 : 
*********************************************************************************************************
*/
#ifndef __HELLO_TASK_H
#define	__HELLO_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
// #include "esp_wpa2.h"
// #include "esp_event.h"
#include "esp_log.h"
// #include "esp_system.h"
// #include "nvs_flash.h"
// #include "esp_netif.h"
// #include "crc16.h"
// #include "bsp_flash.h"
#include "mqtt.h"
#include "bsp_gpio.h"

void hello_task(void * parm);
void timer_task(void *pvParameter);
extern EventGroupHandle_t hello_task_event_group;
#ifdef __cplusplus
}
#endif


#endif