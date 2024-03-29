
// #include "main.h"
#include "mqtt.h"
#include "bsp_gpio.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "dht11.h"
#include <stdbool.h>

static const char *TAG = "mqtt";
esp_mqtt_client_handle_t client = NULL;
bool mqtt_connected = false;

static void log_error_if_nonzero(const char * message, int error_code);

void publish_tem_hum_task(void *pvParameters)
{
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t) pvParameters;
    // char tempStr[10];
    // char humiStr[10];
    // static uint8_t lastTemp = 0;
    // static uint8_t lastHumi = 0;

    // 等待MQTT客户端连接到服务器
    while (!mqtt_connected) {
        vTaskDelay(100 / portTICK_PERIOD_MS); // 每100ms检查一次
    }
    char msg[100];
    while(1)
    {       
    struct dht11_reading last_reading = {0, 0, 0};

    while(1)
    {
        struct dht11_reading reading = DHT11_read();
        if(reading.status == DHT11_OK && (reading.temperature != last_reading.temperature || reading.humidity != last_reading.humidity))
        {
            // 如果温度为44，湿度为122，则忽略这个值
            if(reading.temperature > 40 || reading.humidity > 100)
            // if(reading.temperature == 40 && reading.humidity == 100)
            {
                ESP_LOGI(TAG, "DHT11_ReadError");
                continue;
            }
            last_reading = reading;
            snprintf(msg, sizeof(msg), "Temperature: %d C, Humidity: %d %%", reading.temperature, reading.humidity);

            // 等待MQTT客户端连接到服务器
            while (!mqtt_connected) {
                vTaskDelay(100 / portTICK_PERIOD_MS); // 每100ms检查一次
            }

            // 发布温度和湿度数据
            esp_mqtt_client_publish(client, "/topic/t_h", msg, 0, 0, 1);
            led_status();
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS); // 每5秒读取并发布一次
    }
    }
}

void reset_wifi() {
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

    sprintf(mac_addr, "2shFQFP5hpmPHYSr_%02X%02X%02X%02X%02X%02X_esp32s3", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void erase_wifi_config() {
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
        esp_mqtt_client_publish(client, "/topic/test3", "Helllo World", 0, 0, 0);
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
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d/n", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d/n", msg_id); 
        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d/n", msg_id);
        msg_id = esp_mqtt_client_subscribe(client, "/topic/t_h", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d/n", msg_id);
        xTaskCreate(publish_tem_hum_task, "publish_tem_hum_task", 2048, (void*)client, 5, NULL);    
        // xTaskCreate(Publisher_Task, "Publisher_Task", 1024 * 5, NULL, 5, NULL);
        break;
    case MQTT_EVENT_DISCONNECTED:
        mqtt_connected = false;  
        // led_off();
        esp_mqtt_client_start(client);// 尝试重新连接
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/iot/2742/de", "data", 0, 0, 0);
        msg_id = esp_mqtt_client_publish(client, "/topic/test3", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        led_status();
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        // xTaskCreate(publish_tem_hum_task, "publish_tem_hum_task", 2048, (void*)client, 5, NULL);
        // xTaskCreate(dht11_task, "dht11_task", 2048, (void*)client, 5, NULL);
         //控制LED
            if(memcmp(event->data,"On",event->data_len)==0)
            {
                led_on();
            }else if(memcmp(event->data,"Off",event->data_len)==0)
            {
                led_off();
            }else if (memcmp(event->data, "clean", event->data_len) == 0)
                {                    
                    reset_wifi();// 重置wifi
                } 
            else if (memcmp(event->data, "data_3", event->data_len) == 0)
                {                    
                    led_status();
                // } 
            // else if (strncmp(event->topic, "/topic/t_h", event->topic_len) == 0){
            //     char* temp_value = malloc(event->data_len+1);
            //     memcpy(temp_value, event->data, event->data_len);
            //     temp_value[event->data_len] = '\0';
            //     printf("Received data on /topic/t_h: %s\n", temp_value);
            //     free(temp_value);
            // }
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
    char device_id[32];
    // char client_id[40];
    get_mac_address(device_id);
    // sprintf(client_id, "%s_ESP32S3", device_id);
    esp_mqtt_client_config_t mqtt_cfg = {
        // .broker.address.uri  = "mqtt://yuwawa.vip",
        // .broker.address.uri  = "mqtt://mq.tongxinmao.com",
        // .broker.address.port = 18830,
        
        // .broker.address.uri  = "mqtt://t.yoyolife.fun",
        // .broker.address.port = 1883,
        // .credentials.client_id = "5ddad3c8554e8f74fd3f96ff959dd894",
        // .credentials.username = "5ddad3c8554e8f74fd3f96ff959dd894",
        // .credentials.authentication.password = "123456",

        .broker.address.uri  = "mqtt://admin.yuwawa.vip",
        .broker.address.port = 1883,
        .credentials.client_id = device_id,
        // .credentials.username = "test1",
        // .credentials.authentication.password = "openiitasecret0",
        
        // .broker.address.uri  = "mqtt://broker.emqx.io",
        // .broker.address.port = 1883,
        // .credentials.client_id = device_id,
        // .credentials.client_id = "5ddad3c8554e8f74fd3f96ff959dd894",
        // .credentials.username = "5ddad3c8554e8f74fd3f96ff959dd894",
        // .credentials.authentication.password = "123456",
        // .credentials.client_id = "esp32_client", // 客户端ID，通常需要唯一
        .session.keepalive = 60, // 心跳保活间隔
    };
    
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
