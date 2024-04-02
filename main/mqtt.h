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

#include <stdbool.h>
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
static void erase_wifi_config();
void Publisher_Task(void *params);
extern esp_mqtt_client_handle_t client;
extern bool mqtt_connected;
void publish_tem_hum_task(void *pvParameters);
static void generate_md5_hash(char *input, char output[33]);
static void generate_device_hash(char *device_id, char output[33]);
void publish_tem_hum_task(void *pvParameters);
void get_mac_address(char *mac_addr);
extern char topic[];
extern char user_id[];

#ifdef __cplusplus
}
#endif


#endif