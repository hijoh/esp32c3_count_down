/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/****************************************************************************
* This is a demo for bluetooth config wifi connection to ap. You can config ESP32 to connect a softap
* or config ESP32 as a softap to be connected by other device. APP can be downloaded from github
* android source code: https://github.com/EspressifApp/EspBlufi
* iOS source code: https://github.com/EspressifApp/EspBlufiForiOS
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "hal/gpio_types.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_blufi_api.h"
#include "blufi_example.h"
#include "esp_blufi.h"
#include "mqtt.h"
#include "bsp_gpio.h"
#include "dht11.h"
#include "sdkconfig.h"

#include "freertos/semphr.h"
#include "lv_examples/src/lv_demo_widgets/lv_demo_widgets.h"
#include "lv_examples/src/lv_demo_music/lv_demo_music.h"
#include "lv_examples/src/lv_demo_benchmark/lv_demo_benchmark.h"
#include "lvgl_helpers.h"
#include "esp_freertos_hooks.h"
#include "gui_task.h"
#include "ui.h"
#include "../lvgl/lvgl.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "hello_task.h"

#define EXAMPLE_WIFI_CONNECTION_MAXIMUM_RETRY CONFIG_EXAMPLE_WIFI_CONNECTION_MAXIMUM_RETRY
#define EXAMPLE_INVALID_REASON                255
#define EXAMPLE_INVALID_RSSI                  -128

static const char *TAG = "main";
static void example_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param);
// esp_mqtt_client_handle_t client;
#define WIFI_LIST_NUM   10
// static const char *TAG = "WiFi_Task";
static wifi_config_t sta_config;
static wifi_config_t ap_config;

// EventGroupHandle_t wifiEventGroup;
const int CONNECTED_BIT = BIT0;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */


static uint8_t example_wifi_retry = 0;

/* store the station info for send back to phone */
static bool gl_sta_connected = false;
static bool gl_sta_got_ip = false;
static bool ble_is_connected = false;
static uint8_t gl_sta_bssid[6];
static uint8_t gl_sta_ssid[32];
static int gl_sta_ssid_len;
static wifi_sta_list_t gl_sta_list;
static bool gl_sta_is_connecting = false;
static esp_blufi_extra_info_t gl_sta_conn_info;

static void example_record_wifi_conn_info(int rssi, uint8_t reason)
{
    memset(&gl_sta_conn_info, 0, sizeof(esp_blufi_extra_info_t));
    if (gl_sta_is_connecting) {
        gl_sta_conn_info.sta_max_conn_retry_set = true;
        gl_sta_conn_info.sta_max_conn_retry = EXAMPLE_WIFI_CONNECTION_MAXIMUM_RETRY;
    } else {
        gl_sta_conn_info.sta_conn_rssi_set = true;
        gl_sta_conn_info.sta_conn_rssi = rssi;
        gl_sta_conn_info.sta_conn_end_reason_set = true;
        gl_sta_conn_info.sta_conn_end_reason = reason;
    }
}

static void example_wifi_connect(void)
{
    example_wifi_retry = 0;
    gl_sta_is_connecting = (esp_wifi_connect() == ESP_OK);
    example_record_wifi_conn_info(EXAMPLE_INVALID_RSSI, EXAMPLE_INVALID_REASON);
}

static bool example_wifi_reconnect(void)
{
    bool ret;
    if (gl_sta_is_connecting && example_wifi_retry++ < EXAMPLE_WIFI_CONNECTION_MAXIMUM_RETRY) {
        BLUFI_INFO("BLUFI WiFi starts reconnection\n");
        gl_sta_is_connecting = (esp_wifi_connect() == ESP_OK);
        example_record_wifi_conn_info(EXAMPLE_INVALID_RSSI, EXAMPLE_INVALID_REASON);
        ret = true;
    } else {
        ret = false;
    }
    return ret;
}

static int softap_get_current_connection_number(void)
{
    esp_err_t ret;
    ret = esp_wifi_ap_get_sta_list(&gl_sta_list);
    if (ret == ESP_OK)
    {
        return gl_sta_list.num;
    }

    return 0;
}

static void ip_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    wifi_mode_t mode;

    switch (event_id) {
    case IP_EVENT_STA_GOT_IP: {
        esp_blufi_extra_info_t info;

        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        esp_wifi_get_mode(&mode);

        memset(&info, 0, sizeof(esp_blufi_extra_info_t));
        memcpy(info.sta_bssid, gl_sta_bssid, 6);
        info.sta_bssid_set = true;
        info.sta_ssid = gl_sta_ssid;
        info.sta_ssid_len = gl_sta_ssid_len;
        gl_sta_got_ip = true;
        if (ble_is_connected == true) {
            esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_SUCCESS, softap_get_current_connection_number(), &info);
        } else {
            BLUFI_INFO("BLUFI BLE is not connected yet\n");
        }
        break;
    }
    default:
        break;
    }
    return;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    wifi_event_sta_connected_t *event;
    wifi_event_sta_disconnected_t *disconnected_event;
    wifi_mode_t mode;

    switch (event_id) {
    case WIFI_EVENT_STA_START:
        example_wifi_connect();
        break;
    case WIFI_EVENT_STA_CONNECTED:
        gl_sta_connected = true;
        gl_sta_is_connecting = false;
        event = (wifi_event_sta_connected_t*) event_data;
        memcpy(gl_sta_bssid, event->bssid, 6);
        memcpy(gl_sta_ssid, event->ssid, event->ssid_len);
        gl_sta_ssid_len = event->ssid_len;
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        /* Only handle reconnection during connecting */
        if (gl_sta_connected == false && example_wifi_reconnect() == false) {
            gl_sta_is_connecting = false;
            disconnected_event = (wifi_event_sta_disconnected_t*) event_data;
            example_record_wifi_conn_info(disconnected_event->rssi, disconnected_event->reason);
        }
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        gl_sta_connected = false;
        gl_sta_got_ip = false;
        memset(gl_sta_ssid, 0, 32);
        memset(gl_sta_bssid, 0, 6);
        gl_sta_ssid_len = 0;
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    case WIFI_EVENT_AP_START:
        esp_wifi_get_mode(&mode);

        /* TODO: get config or information of softap, then set to report extra_info */
        if (ble_is_connected == true) {
            if (gl_sta_connected) {
                esp_blufi_extra_info_t info;
                memset(&info, 0, sizeof(esp_blufi_extra_info_t));
                memcpy(info.sta_bssid, gl_sta_bssid, 6);
                info.sta_bssid_set = true;
                info.sta_ssid = gl_sta_ssid;
                info.sta_ssid_len = gl_sta_ssid_len;
                esp_blufi_send_wifi_conn_report(mode, gl_sta_got_ip ? ESP_BLUFI_STA_CONN_SUCCESS : ESP_BLUFI_STA_NO_IP, softap_get_current_connection_number(), &info);
            } else if (gl_sta_is_connecting) {
                esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONNECTING, softap_get_current_connection_number(), &gl_sta_conn_info);
            } else {
                esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_FAIL, softap_get_current_connection_number(), &gl_sta_conn_info);
            }
        } else {
            BLUFI_INFO("BLUFI BLE is not connected yet\n");
        }
        break;
    case WIFI_EVENT_SCAN_DONE: {
        uint16_t apCount = 0;
        esp_wifi_scan_get_ap_num(&apCount);
        if (apCount == 0) {
            BLUFI_INFO("Nothing AP found");
            break;
        }
        wifi_ap_record_t *ap_list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);
        if (!ap_list) {
            BLUFI_ERROR("malloc error, ap_list is NULL");
            break;
        }
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, ap_list));
        esp_blufi_ap_record_t * blufi_ap_list = (esp_blufi_ap_record_t *)malloc(apCount * sizeof(esp_blufi_ap_record_t));
        if (!blufi_ap_list) {
            if (ap_list) {
                free(ap_list);
            }
            BLUFI_ERROR("malloc error, blufi_ap_list is NULL");
            break;
        }
        for (int i = 0; i < apCount; ++i)
        {
            blufi_ap_list[i].rssi = ap_list[i].rssi;
            memcpy(blufi_ap_list[i].ssid, ap_list[i].ssid, sizeof(ap_list[i].ssid));
        }

        if (ble_is_connected == true) {
            esp_blufi_send_wifi_list(apCount, blufi_ap_list);
        } else {
            BLUFI_INFO("BLUFI BLE is not connected yet\n");
        }

        esp_wifi_scan_stop();
        free(ap_list);
        free(blufi_ap_list);
        break;
    }
    case WIFI_EVENT_AP_STACONNECTED: {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        BLUFI_INFO("station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
        break;
    }
    case WIFI_EVENT_AP_STADISCONNECTED: {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        BLUFI_INFO("station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
        break;
    }

    default:
        break;
    }
    return;
}

static void initialise_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());//初始化网络接口
    wifi_event_group = xEventGroupCreate();//创建事件组
    ESP_ERROR_CHECK(esp_event_loop_create_default());//创建默认事件循环
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();//创建默认的Wi-Fi站点接口
    assert(sta_netif);//使用assert()宏对接口的创建结果进行断言
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();//创建默认的Wi-Fi接入点接口
    assert(ap_netif);//使用assert()宏对接口的创建结果进行断言
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));//注册Wi-Fi事件处理程序
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));//注册IP事件处理程序

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();//初始化Wi-Fi配置
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );//初始化Wi-Fi
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );//设置Wi-Fi模式
    example_record_wifi_conn_info(EXAMPLE_INVALID_RSSI, EXAMPLE_INVALID_REASON);//记录无效的Wi-Fi连接信息
    ESP_ERROR_CHECK( esp_wifi_start() );//启动Wi-Fi
}

static esp_blufi_callbacks_t example_callbacks = {
    .event_cb = example_event_callback,
    .negotiate_data_handler = blufi_dh_negotiate_data_handler,
    .encrypt_func = blufi_aes_encrypt,
    .decrypt_func = blufi_aes_decrypt,
    .checksum_func = blufi_crc_checksum,
};

static void example_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param)
{
    /* actually, should post to blufi_task handle the procedure,
     * now, as a example, we do it more simply 
     switch (event) {
    case ESP_BLUFI_EVENT_INIT_FINISH:
        // BLUFI 模块初始化完成事件
        break;
    case ESP_BLUFI_EVENT_DEINIT_FINISH:
        // BLUFI 模块反初始化完成事件
        break;
    case ESP_BLUFI_EVENT_BLE_CONNECT:
        // BLE 连接建立事件
        break;
    case ESP_BLUFI_EVENT_BLE_DISCONNECT:
        // BLE 连接断开事件
        break;
    case ESP_BLUFI_EVENT_SET_WIFI_OPMODE:
        // 设置 Wi-Fi 工作模式事件
        break;
    case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
        // 请求连接到 Wi-Fi 接入点事件
        break;
    case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
        // 请求从当前连接的 Wi-Fi 接入点断开事件
        break;
    case ESP_BLUFI_EVENT_REPORT_ERROR:
        // BLUFI 模块报告错误事件
        break;
    case ESP_BLUFI_EVENT_GET_WIFI_STATUS:
        // 请求获取当前 Wi-Fi 连接状态事件
        break;
    case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
        // GATT 连接关闭事件
        break;
    case ESP_BLUFI_EVENT_DEAUTHENTICATE_STA:
        // STA 被取消认证事件
        break;
    case ESP_BLUFI_EVENT_RECV_STA_BSSID:
        // 接收到 STA 的 BSSID（MAC 地址）事件
        break;
    case ESP_BLUFI_EVENT_RECV_STA_SSID:
        // 接收到 STA 的 SSID（Wi-Fi 网络名称）事件
        break;
    case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
        // 接收到 STA 的密码事件
        break;
    case ESP_BLUFI_EVENT_RECV_SOFTAP_SSID:
        // 接收到 SoftAP 的 SSID（Wi-Fi 网络名称）事件
        break;
    case ESP_BLUFI_EVENT_RECV_SOFTAP_PASSWD:
        // 接收到 SoftAP 的密码事件
        break;
    case ESP_BLUFI_EVENT_RECV_SOFTAP_MAX_CONN_NUM:
        // 接收到 SoftAP 的最大连接数事件
        break;
    case ESP_BLUFI_EVENT_RECV_SOFTAP_AUTH_MODE:
        // 接收到 SoftAP 的认证模式事件
        break;
    case ESP_BLUFI_EVENT_RECV_SOFTAP_CHANNEL:
        // 接收到 SoftAP 的信道事件
        break;
    case ESP_BLUFI_EVENT_GET_WIFI_LIST:
        // 请求获取可用的 Wi-Fi 网络列表事件
        break;
    case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA:
        // 接收到自定义数据事件
        break;
    case ESP_BLUFI_EVENT_RECV_USERNAME:
        // 接收到用户名事件
        break;
    case ESP_BLUFI_EVENT_RECV_CA_CERT:
        // 接收到 CA（证书颁发机构）证书事件
        break;
    case ESP_BLUFI_EVENT_RECV_CLIENT_CERT:
        // 接收到客户端证书事件
        break;
    case ESP_BLUFI_EVENT_RECV_SERVER_CERT:
        // 接收到服务器证书事件
        break;
    case ESP_BLUFI_EVENT_RECV_CLIENT_PRIV_KEY:
        // 接收到客户端私钥事件
        break;
    case ESP_BLUFI_EVENT_RECV_SERVER_PRIV_KEY:
        // 接收到服务器私钥事件
        break;
    default:
        break;
}
     */
    switch (event) {
    case ESP_BLUFI_EVENT_INIT_FINISH:
        BLUFI_INFO("BLUFI init finish\n");

        esp_blufi_adv_start();
        break;
    case ESP_BLUFI_EVENT_DEINIT_FINISH:
        BLUFI_INFO("BLUFI deinit finish\n");
        break;
    case ESP_BLUFI_EVENT_BLE_CONNECT:
        BLUFI_INFO("BLUFI ble connect\n");
        ble_is_connected = true;
        esp_blufi_adv_stop();
        blufi_security_init();
        break;
    case ESP_BLUFI_EVENT_BLE_DISCONNECT:
        BLUFI_INFO("BLUFI ble disconnect\n");
        ble_is_connected = false;
        blufi_security_deinit();
        esp_blufi_adv_start();
        break;
    case ESP_BLUFI_EVENT_SET_WIFI_OPMODE:
        BLUFI_INFO("BLUFI Set WIFI opmode %d\n", param->wifi_mode.op_mode);
        ESP_ERROR_CHECK( esp_wifi_set_mode(param->wifi_mode.op_mode) );
        break;
    case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
        BLUFI_INFO("BLUFI requset wifi connect to AP\n");
        /* there is no wifi callback when the device has already connected to this wifi
        so disconnect wifi before connection.
        */
        esp_wifi_disconnect();
        example_wifi_connect();
        break;
    case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
        BLUFI_INFO("BLUFI requset wifi disconnect from AP\n");
        esp_wifi_disconnect();
        break;
    case ESP_BLUFI_EVENT_REPORT_ERROR:
        BLUFI_ERROR("BLUFI report error, error code %d\n", param->report_error.state);
        esp_blufi_send_error_info(param->report_error.state);
        break;
    case ESP_BLUFI_EVENT_GET_WIFI_STATUS: {
        wifi_mode_t mode;
        esp_blufi_extra_info_t info;

        esp_wifi_get_mode(&mode);

        if (gl_sta_connected) {
            memset(&info, 0, sizeof(esp_blufi_extra_info_t));
            memcpy(info.sta_bssid, gl_sta_bssid, 6);
            info.sta_bssid_set = true;
            info.sta_ssid = gl_sta_ssid;
            info.sta_ssid_len = gl_sta_ssid_len;
            esp_blufi_send_wifi_conn_report(mode, gl_sta_got_ip ? ESP_BLUFI_STA_CONN_SUCCESS : ESP_BLUFI_STA_NO_IP, softap_get_current_connection_number(), &info);
        } else if (gl_sta_is_connecting) {
            esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONNECTING, softap_get_current_connection_number(), &gl_sta_conn_info);
        } else {
            esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_FAIL, softap_get_current_connection_number(), &gl_sta_conn_info);
        }
        BLUFI_INFO("BLUFI get wifi status from AP\n");

        break;
    }
    case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
        BLUFI_INFO("blufi close a gatt connection");
        esp_blufi_disconnect();
        break;
    case ESP_BLUFI_EVENT_DEAUTHENTICATE_STA:
        /* TODO */
        break;
	case ESP_BLUFI_EVENT_RECV_STA_BSSID:
        memcpy(sta_config.sta.bssid, param->sta_bssid.bssid, 6);
        sta_config.sta.bssid_set = 1;
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        BLUFI_INFO("Recv STA BSSID %s\n", sta_config.sta.ssid);
        break;
	case ESP_BLUFI_EVENT_RECV_STA_SSID:
        strncpy((char *)sta_config.sta.ssid, (char *)param->sta_ssid.ssid, param->sta_ssid.ssid_len);
        sta_config.sta.ssid[param->sta_ssid.ssid_len] = '\0';
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        BLUFI_INFO("Recv STA SSID %s\n", sta_config.sta.ssid);
        break;
	case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
        strncpy((char *)sta_config.sta.password, (char *)param->sta_passwd.passwd, param->sta_passwd.passwd_len);
        sta_config.sta.password[param->sta_passwd.passwd_len] = '\0';
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        BLUFI_INFO("Recv STA PASSWORD %s\n", sta_config.sta.password);
        break;
	case ESP_BLUFI_EVENT_RECV_SOFTAP_SSID:
        strncpy((char *)ap_config.ap.ssid, (char *)param->softap_ssid.ssid, param->softap_ssid.ssid_len);
        ap_config.ap.ssid[param->softap_ssid.ssid_len] = '\0';
        ap_config.ap.ssid_len = param->softap_ssid.ssid_len;
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        BLUFI_INFO("Recv SOFTAP SSID %s, ssid len %d\n", ap_config.ap.ssid, ap_config.ap.ssid_len);
        break;
	case ESP_BLUFI_EVENT_RECV_SOFTAP_PASSWD:
        strncpy((char *)ap_config.ap.password, (char *)param->softap_passwd.passwd, param->softap_passwd.passwd_len);
        ap_config.ap.password[param->softap_passwd.passwd_len] = '\0';
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        BLUFI_INFO("Recv SOFTAP PASSWORD %s len = %d\n", ap_config.ap.password, param->softap_passwd.passwd_len);
        break;
	case ESP_BLUFI_EVENT_RECV_SOFTAP_MAX_CONN_NUM:
        if (param->softap_max_conn_num.max_conn_num > 4) {
            return;
        }
        ap_config.ap.max_connection = param->softap_max_conn_num.max_conn_num;
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        BLUFI_INFO("Recv SOFTAP MAX CONN NUM %d\n", ap_config.ap.max_connection);
        break;
	case ESP_BLUFI_EVENT_RECV_SOFTAP_AUTH_MODE:
        if (param->softap_auth_mode.auth_mode >= WIFI_AUTH_MAX) {
            return;
        }
        ap_config.ap.authmode = param->softap_auth_mode.auth_mode;
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        BLUFI_INFO("Recv SOFTAP AUTH MODE %d\n", ap_config.ap.authmode);
        break;
	case ESP_BLUFI_EVENT_RECV_SOFTAP_CHANNEL:
        if (param->softap_channel.channel > 13) {
            return;
        }
        ap_config.ap.channel = param->softap_channel.channel;
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        BLUFI_INFO("Recv SOFTAP CHANNEL %d\n", ap_config.ap.channel);
        break;
    case ESP_BLUFI_EVENT_GET_WIFI_LIST:{
        wifi_scan_config_t scanConf = {
            .ssid = NULL,
            .bssid = NULL,
            .channel = 0,
            .show_hidden = false
        };
        esp_err_t ret = esp_wifi_scan_start(&scanConf, true);
        if (ret != ESP_OK) {
            esp_blufi_send_error_info(ESP_BLUFI_WIFI_SCAN_FAIL);
        }
        break;
    }
    case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA:
        BLUFI_INFO("Recv Custom Data %" PRIu32 "\n", param->custom_data.data_len);
        esp_log_buffer_hex("Custom Data", param->custom_data.data, param->custom_data.data_len);
            // 如果接收到的数据是9，重启ESP
            
            if (param->custom_data.data_len == 1 && param->custom_data.data[0] == '9') {
    int value = 0;
    ESP_LOGI(TAG, "Received value: %d", value);

    esp_mqtt_client_stop(client);
    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

        break;
	case ESP_BLUFI_EVENT_RECV_USERNAME:
        /* Not handle currently */
        break;
	case ESP_BLUFI_EVENT_RECV_CA_CERT:
        /* Not handle currently */
        break;
	case ESP_BLUFI_EVENT_RECV_CLIENT_CERT:
        /* Not handle currently */
        break;
	case ESP_BLUFI_EVENT_RECV_SERVER_CERT:
        /* Not handle currently */
        break;
	case ESP_BLUFI_EVENT_RECV_CLIENT_PRIV_KEY:
        /* Not handle currently */
        break;;
	case ESP_BLUFI_EVENT_RECV_SERVER_PRIV_KEY:
        /* Not handle currently */
        break;
    default:
        break;
    }
}
void wifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        // WiFi成功连接
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
}
void wifi_task(void *pvParameters) {
    // bool is_initialized = false; // 添加一个标志位
    // while (1) {
    //     // 检查Wi-Fi连接状态
    //     wifi_ap_record_t ap_info;
    //     esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    //     if (ret == ESP_OK) {
    //         ESP_LOGI(TAG, "Wi-Fi connected, SSID: %s", ap_info.ssid);
    //         // 如果还没有初始化，就执行初始化操作
    //         if (!is_initialized) {
    //             // 初始化GPIO
    //             bsp_gpio_init();
    //             // 启动mqtt
    //             mqtt_start();
    //             is_initialized = true; // 标志位设为true，表示已经初始化过了
    //         }
    //     } else {
    //         ESP_LOGI(TAG, "Wi-Fi not connected");
    //         is_initialized = false; // 如果Wi-Fi断开，标志位设为false
    //     }

    //     vTaskDelay(pdMS_TO_TICKS(5000)); // 每隔5秒检查一次Wi-Fi连接状态
    // }

        // 创建事件组
    wifi_event_group = xEventGroupCreate();

    // 注册WiFi事件处理程序
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, NULL);

    // 配置WiFi连接和初始化
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    // 等待WiFi成功连接的事件
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    //WiFi连接成功后的处理逻辑
    // bsp_gpio_init();
    
    while (1)
    {
        // 任务的主循环逻辑
        // ESP_LOGI(TAG, "wifi_task_is_running");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    xTaskCreatePinnedToCore(gui_task, "gui task", 1024 * 5, NULL, 15, NULL, 1);
    esp_rom_gpio_pad_select_gpio(GPIO_NUM_2);
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 1);

    esp_err_t ret;

    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    initialise_wifi();

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));//释放经典蓝牙资源

    //初始化蓝牙控制器：创建一个esp_bt_controller_config_t类型的结构体bt_cfg，
    // 并使用BT_CONTROLLER_INIT_CONFIG_DEFAULT()宏来初始化该结构体，
    // 然后调用esp_bt_controller_init()函数来初始化蓝牙控制器。
    // 如果初始化失败，会打印错误信息。
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        BLUFI_ERROR("%s initialize bt controller failed: %s\n", __func__, esp_err_to_name(ret));
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);//启用BLE模式
    if (ret) {
        BLUFI_ERROR("%s enable bt controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_blufi_host_and_cb_init(&example_callbacks);//初始化BLE服务和回调函数
    if (ret) {
        BLUFI_ERROR("%s initialise failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    
    // mqtt_start();// 启动mqtt
    DHT11_init(GPIO_NUM_9);
    // global_message_mutex = xSemaphoreCreateMutex();//初始化global_message_mutex
    // DHT11_Init();
    // DHT11_Start();
    BLUFI_INFO("BLUFI VERSION %04x\n", esp_blufi_get_version());//打印BLUFI版本信息
    // xTaskCreate(&wifi_task, "wifi_task", 1024*8, NULL, 4, NULL);
    xTaskCreate(hello_task, "hello_task", 4096, NULL, 4, NULL);
    xTaskCreate(publish_tem_hum_task, "publish_tem_hum_task", 2048, (void*)client, 5, NULL);    
    // xTaskCreate(Publisher_Task, "Publisher_Task", 1024 * 5, NULL, 5, NULL);
    
    // xTaskCreate(&button_check_task, "button_check_task", 2048, NULL, 5, NULL);

    // xTaskCreate(publish_tem_hum_task, "publish_tem_hum_task", 2048, (void*)client, 5, NULL);
    // xTaskCreate(publish_tem_hum_task, "publish_tem_hum_task", 2048, NULL, 5, NULL);
    // xTaskCreate(&dht11_task, "dht11_task", configMINIMAL_STACK_SIZE * 4, NULL, 6, NULL);
    // xTaskCreate(dht11_task, "dht11_task", 2048, (void*)client, 5, NULL);

}
