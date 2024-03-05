/*
*********************************************************************************************************
*
*	模块名称 : mqtt client
*	文件名称 : mqtt.c
*	版    本 : V1.0
*	说    明 : 连mqtt服务器
*   联系方式  ：嘉友创-小步（微信：15962207161）
*
*	修改记录 :
*	版本号          日期        		作者     		说明
*	V1.0           2022-10-07 		  DOUBLE  		 正式发布
*********************************************************************************************************
*/
// #include "main.h"
#include "mqtt.h"
#include "bsp_gpio.h"

static const char *TAG = "mqtt";

esp_mqtt_client_handle_t client;
//MQTT信息

static void log_error_if_nonzero(const char * message, int error_code);

void enter_smartconfig() {
    // 停止MQTT连接（如果正在连接）
    esp_mqtt_client_stop(client);

    // 停止WiFi连接
    esp_wifi_stop();
/*
        _user_wifi_data.en = 0;
    memset(_user_wifi_data.ssid,0,sizeof(_user_wifi_data.ssid));
    memset(_user_wifi_data.pw,0,sizeof(_user_wifi_data.pw));
    CRCXMODEM_Make_CRC(&_user_wifi_data.en, sizeof(user_wifi_data) - 2, 0xffff);
    write_user_flash(&_user_wifi_data.en, sizeof(user_wifi_data));
    */
    ESP_LOGI(TAG, "flash data init OK");

    // 重新启动设备
    esp_restart();
}

void get_mac_address(char *mac_addr) {
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac); // 获取STA模式的MAC地址

    sprintf(mac_addr, "2shFQFP5hpmPHYSr_%02X%02X%02X%02X%02X%02X_esp", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}



/*
*********************************************************************************************************
*	函 数 名: mqtt_event_handler_cb
*	功能说明: mqtt事件回调
*	形    参：socket
*	返 回 值: 无
*********************************************************************************************************
*/
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
         //控制LED
            if(memcmp(event->data,"On",event->data_len)==0)
            {
                led_on();
            }else if(memcmp(event->data,"Off",event->data_len)==0)
            {
                led_off();
            }else if (memcmp(event->data, "clean", event->data_len) == 0)
                {
                    // 重新进入SmartConfig
                    enter_smartconfig();
                } 
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void log_error_if_nonzero(const char * message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}
/*
*********************************************************************************************************
*	函 数 名: mqtt_start
*	功能说明: 启动mqtt
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void mqtt_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri  = "mqtt://yuwawa.vip",
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}
