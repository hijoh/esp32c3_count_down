
#include "esp_timer.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dht11.h"
#include "mqtt.h"
#include "bsp_gpio.h"
#include "esp_log.h"

#define aht20_addr 0x38
static const char *TAG = "DHT11";

static gpio_num_t dht_gpio;
static int64_t last_read_time = -2000000;
static struct dht11_reading last_read;

static int _waitOrTimeout(uint16_t microSeconds, int level) {
    int micros_ticks = 0;
    while(gpio_get_level(dht_gpio) == level) { 
        if(micros_ticks++ > microSeconds) 
            return DHT11_TIMEOUT_ERROR;
        ets_delay_us(1);
    }
    return micros_ticks;
}

static int _checkCRC(uint8_t data[]) {
    if(data[4] == (data[0] + data[1] + data[2] + data[3]))
        return DHT11_OK;
    else
        return DHT11_CRC_ERROR;
}

static void _sendStartSignal() {
    gpio_set_direction(dht_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(dht_gpio, 0);
    ets_delay_us(20 * 1000);
    gpio_set_level(dht_gpio, 1);
    ets_delay_us(40);
    gpio_set_direction(dht_gpio, GPIO_MODE_INPUT);
}

static int _checkResponse() {
    /* Wait for next step ~80us*/
    if(_waitOrTimeout(80, 0) == DHT11_TIMEOUT_ERROR)
        return DHT11_TIMEOUT_ERROR;

    /* Wait for next step ~80us*/
    if(_waitOrTimeout(80, 1) == DHT11_TIMEOUT_ERROR) 
        return DHT11_TIMEOUT_ERROR;

    return DHT11_OK;
}

static struct dht11_reading _timeoutError() {
    struct dht11_reading timeoutError = {DHT11_TIMEOUT_ERROR, -1, -1};
    return timeoutError;
}

static struct dht11_reading _crcError() {
    struct dht11_reading crcError = {DHT11_CRC_ERROR, -1, -1};
    return crcError;
}

void DHT11_init(gpio_num_t gpio_num) {
    /* Wait 1 seconds to make the device pass its initial unstable status */
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    dht_gpio = gpio_num;
}

struct dht11_reading DHT11_read() {
    /* Tried to sense too son since last read (dht11 needs ~2 seconds to make a new read) */
    if(esp_timer_get_time() - 2000000 < last_read_time) {
        return last_read;
    }

    last_read_time = esp_timer_get_time();

    uint8_t data[5] = {0,0,0,0,0};

    _sendStartSignal();

    if(_checkResponse() == DHT11_TIMEOUT_ERROR)
        return last_read = _timeoutError();
    
    /* Read response */
    for(int i = 0; i < 40; i++) {
        /* Initial data */
        if(_waitOrTimeout(50, 0) == DHT11_TIMEOUT_ERROR)
            return last_read = _timeoutError();
                
        if(_waitOrTimeout(70, 1) > 28) {
            /* Bit received was a 1 */
            data[i/8] |= (1 << (7-(i%8)));
        }
    }

    if(_checkCRC(data) != DHT11_CRC_ERROR) {
        last_read.status = DHT11_OK;
        last_read.temperature = data[2];
        last_read.humidity = data[0];
        return last_read;
    } else {
        return last_read = _crcError();
    }
}

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