
// #include "main.h"
#include "mqtt.h"
#include "bsp_gpio.h"
#include "esp_rom_md5.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdbool.h>
#include "mbedtls/md5.h"
#include "cJSON.h"

static const char *TAG = "mqtt";
esp_mqtt_client_handle_t client = NULL;
bool mqtt_connected = false;

char user_id[13];
char topic[100];

static void log_error_if_nonzero(const char * message, int error_code);

// 生成MD5哈希
static void generate_md5_hash(char *input, char output[33]) {
    unsigned char digest[16];
    mbedtls_md5_context ctx;
    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, (unsigned char *)input, strlen(input));
    mbedtls_md5_finish(&ctx, digest);
    mbedtls_md5_free(&ctx);

    for(int i = 0; i < 16; ++i)
        sprintf(&output[i*2], "%02x", (unsigned int)digest[i]);
}

// 拼接字符串和设备ID，然后生成MD5哈希
static void generate_device_hash(char *device_id, char output[33]) {
    char input[64] = "openiitasecret01";
    strcat(input, device_id);
    generate_md5_hash(input, output);
}

static void reset_wifi() {
    // 停止MQTT连接（如果正在连接）
    esp_mqtt_client_stop(client);

    esp_wifi_disconnect();// 停止WiFi连接 
    erase_wifi_config();
    wifi_config_t wifi_config = {
    .sta = {
        .ssid = "",
        .password = "",
        },
    };
    esp_err_t wifi_config_msg = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (wifi_config_msg != ESP_OK) {
    ESP_LOGI(TAG, "wifi_config_msg !=esp_OK");
    }
    // erase_wifi_config();

    ESP_LOGI(TAG, "flash data init OK");

    // 重新启动设备
    esp_restart();
}

void get_mac_address(char *mac_addr) {
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac); // 获取STA模式的MAC地址
    sprintf(mac_addr, "openiitagateway01_%02X%02X%02X%02X%02X%02X_C3", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    // sprintf(mac_addr, "%02X%02X%02X%02X%02X%02X_S3", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void get_user_id(char *user_id) {
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac); // 获取STA模式的MAC地址
    sprintf(user_id, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void erase_wifi_config() {
    nvs_handle_t my_handle;
    esp_err_t err;

    // 初始化NVS
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS分区被损坏，需要擦除整个分区
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // 打开NVS
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        // 擦除"ssid"和"password"
        err = nvs_erase_key(my_handle, "ssid");
        if (err != ESP_OK) {
            printf("Error (%s) erasing NVS ssid!\n", esp_err_to_name(err));
        }
        err = nvs_erase_key(my_handle, "password");
        if (err != ESP_OK) {
            printf("Error (%s) erasing NVS password!\n", esp_err_to_name(err));
        }
        //提交NVS
        nvs_commit( my_handle);

        // 关闭NVS
        nvs_close(my_handle);
    }
}

void Publisher_Task(void *params)
{
  while (true)
  {
    if(mqtt_connected)
    {
        
        // ESP_LOGI(TAG, "mqtt_connect: %d", mqtt_connected);
        // esp_mqtt_client_publish(client, "/topic/test3", "Helllo World", 0, 0, 0);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
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
    // esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        mqtt_connected = true;
        led_status();
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 1);
        
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d/n", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/iot/2742/de", 0);
        // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d/n", msg_id);

        // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d/n", msg_id); 
        // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        // ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d/n", msg_id);
        msg_id = esp_mqtt_client_subscribe(client, "/topic/t_h", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d/n", msg_id);


        // sprintf(topic, "/sys/openiitagateway01/%s/c/#",  user_id);
        sprintf(topic, "/openiitagateway01/%s/c/#",  user_id);
        msg_id = esp_mqtt_client_subscribe(client, topic, 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d/n", msg_id);
        
        // xTaskCreate(Publisher_Task, "Publisher_Task", 1024 * 5, NULL, 5, NULL);
        break;
    case MQTT_EVENT_DISCONNECTED:
        mqtt_connected = false; 
        static int reconnect_attempts = 0; 
        led_off();
        esp_err_t err = esp_mqtt_client_reconnect(client);
        if (err != ESP_OK) {
            // mqtt_start();
            reconnect_attempts++; // Increment the counter if reconnection fails
        if (reconnect_attempts >= 5) { // Check if the counter has reached 5
            ESP_LOGI(TAG, "Reconnect error! Restarting after 5 failed attempts.");
            // esp_restart();
        } else {
            ESP_LOGI(TAG, "Reconnect error!mqtt_start()");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");}
        break;
        
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // msg_id = esp_mqtt_client_publish(client, "/iot/2742/de", "data", 0, 0, 0);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        // ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        // led_status();
        break;
    case MQTT_EVENT_DATA:
        // ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        // printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        // printf("DATA=%.*s\r\n", event->data_len, event->data);

        // char* data_str = malloc(event->data_len + 1);
        // memcpy(data_str, event->data, event->data_len);
        // data_str[event->data_len] = '\0';
        // cJSON *data_json = cJSON_Parse(data_str);
        // if (data_json == NULL) {
        //     printf("Error: Could not parse JSON data.\n");
        // } else {
        //     cJSON *led_ctr = cJSON_GetObjectItemCaseSensitive(data_json, "LED_CTR");
        //     if (cJSON_IsTrue(led_ctr)) {
        //         led_on();
        //     } else if (cJSON_IsFalse(led_ctr)) {
        //         led_off();
        //     }
        //     cJSON_Delete(data_json);
        // }
        // free(data_str);
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
{   char device_id[35];
    get_mac_address(device_id);
    get_user_id(user_id);
    char password[33];
    generate_device_hash(device_id,password);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri  = "mqtt://admin.yuwawa.vip",
        // .broker.address.uri  = "mqtt://mq.tongxinmao.com",
        // .broker.address.port = 18830,
        // .broker.address.uri  = "mqtt://t.yoyolife.fun",
        // .broker.address.port = 1883,
        // .credentials.client_id = "5ddad3c8554e8f74fd3f96ff959dd894",
        // .credentials.username = "5ddad3c8554e8f74fd3f96ff959dd894",
        // .credentials.authentication.password = "123456",
        // .broker.address.uri  = "mqtt://admin.yuwawa.vip",
        // .broker.address.port = 1883,

        // .broker.address.uri  = "mqtt://192.168.3.22",
        .broker.address.port = 1883,
        // .credentials.username = user_id,
        // .credentials.authentication.password = password,

        .credentials.client_id = device_id,
        // .credentials.username = "test1",
        // .credentials.authentication.password = "openiitasecret0",
        
        // .broker.address.uri  = "mqtt://broker.emqx.io",
        // .broker.address.port = 1883,
 
        .session.keepalive = 60, // 心跳保活间隔
    };
    // printf("md5_hash:[%s]\n",md5_hash);
    printf("userid:[%s]\n", user_id);
    printf("deviceid:[%s]\n", device_id);
    printf("password:[%s]\n", password);
    
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
    esp_err_t init_result = esp_mqtt_client_start(client);
    if (init_result == ESP_OK) {
        ESP_LOGI(TAG, "MQTT client started successfully");
    } else {
        ESP_LOGE(TAG, "MQTT client initialization failed. Error code: %d", init_result);
        // Handle the failure (e.g., retry, cleanup, etc.)
    }
    // ESP_LOGI(TAG, "mqtt_is_running");
}


