/*
*********************************************************************************************************
*
*	模块名称 : tcp服务器
*	文件名称 : tcp_server.c
*	版    本 : V1.0
*	说    明 : 用于AP配网
*********************************************************************************************************
*/
#ifndef __MQTT_H
#define	__MQTT_H

#ifdef __cplusplus
extern "C" {
#endif


#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "mqtt_client.h"


extern void mqtt_start(void);

#ifdef __cplusplus
}
#endif


#endif