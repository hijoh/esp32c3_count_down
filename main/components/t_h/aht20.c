#include "include/aht20.h"
#include <math.h>
#include "mqtt.h"
#include "cJSON.h"

#define aht20_addr 0x38
struct aht20_data aht20_data_t;
static const char *TAG = "AHT20";

bool getbit(int NUM, int n){
    if(NUM &(1<<n)){return true;}
    else {return false;}
}

void check_calibration(){
    uint8_t status_byte[1] = {0};
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle); //0x71
    i2c_master_write_byte(cmd_handle, (aht20_addr << 1 | I2C_MASTER_READ), true ); //send 0x71
    i2c_master_read_byte(cmd_handle, (uint8_t *) &status_byte, I2C_MASTER_ACK);
    i2c_master_stop(cmd_handle);
    i2c_master_cmd_begin(I2C_NUM_0, cmd_handle, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd_handle);
    if(!(getbit(status_byte[0],3))){
        i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
        i2c_master_start(cmd_handle);
        i2c_master_write_byte(cmd_handle, (aht20_addr << 1 | I2C_MASTER_WRITE), true );
        i2c_master_write_byte(cmd_handle, (0xbe), true);
        i2c_master_write_byte(cmd_handle, (0x08), true);
        i2c_master_write_byte(cmd_handle, (0x00), true);
        i2c_master_stop(cmd_handle);
        i2c_master_cmd_begin(I2C_NUM_0, cmd_handle, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd_handle);
    }
    printf("status %u\n", (uint8_t)status_byte[0]);


        vTaskDelay(100 / portTICK_PERIOD_MS);
}

void trigger_measure(){
     i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
     i2c_master_start(cmd_handle); //0x71
     i2c_master_write_byte(cmd_handle, (aht20_addr << 1 | I2C_MASTER_WRITE), true );
     i2c_master_write_byte(cmd_handle, (0xac), true );
     i2c_master_write_byte(cmd_handle, (0x33), true );
     i2c_master_write_byte(cmd_handle, (0x00), true );
     i2c_master_stop(cmd_handle);
     i2c_master_cmd_begin(I2C_NUM_0, cmd_handle, 800 / portTICK_PERIOD_MS);
     i2c_cmd_link_delete(cmd_handle);
}

static i2c_config_t aht20_i2c_conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = 13, //default pin for SDA
    .scl_io_num = 14, //default pin for SCL
    .sda_pullup_en = GPIO_PULLUP_DISABLE,
    .scl_pullup_en = GPIO_PULLUP_DISABLE,
    .master.clk_speed = 100000
};

void aht20_init(gpio_num_t clock, gpio_num_t data) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    aht20_i2c_conf.sda_io_num = data;
    aht20_i2c_conf.scl_io_num = clock;
}

void i2c_setup(){
    i2c_param_config(I2C_NUM_0, &aht20_i2c_conf);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

void aht20_read_measures(void *ignore){
    
    vTaskDelay(200 / portTICK_PERIOD_MS);
    uint8_t data[7]= {0,0,0,0,0,0,0};
    while(1){
        trigger_measure();
        i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
        i2c_master_start(cmd_handle);
        i2c_master_write_byte(cmd_handle, (aht20_addr << 1 | I2C_MASTER_READ), true );
        i2c_master_read(cmd_handle, (uint8_t *) &data, 7, I2C_MASTER_ACK);
        i2c_master_stop(cmd_handle);
        i2c_master_cmd_begin(I2C_NUM_0, cmd_handle, 800 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd_handle);

        uint32_t rh = ( ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | (data[3]) ) >> 4 ;
        uint32_t temp = ((uint32_t)(data[3]&0x0F) << 16) | ((uint32_t)data[4] << 8) | (uint32_t)data[5] ;

        aht20_data_t.temperature = (temp *(0.00019073F)-50);
        aht20_data_t.rel_humidity = (rh * (0.0000953674316F));
        aht20_data_t.temperature = roundf(aht20_data_t.temperature * 100) / 100;
        aht20_data_t.rel_humidity = roundf(aht20_data_t.rel_humidity * 100) / 100;
        // aht20_data_t.temperature = roundf(aht20_data_t.temperature * 100.0) / 100.0;
        // aht20_data_t.rel_humidity = roundf(aht20_data_t.rel_humidity * 100.0) / 100.0;

        // printf("Temperature is %3.2f\n", (float)temp);
        // printf("Humidity is %3.2f\n", (float)rh);
// 
        // printf("Temperature11 is %3.2f\n", aht20_data_t.temperature);
        // printf("Humidity11 is %3.2f\n", aht20_data_t.rel_humidity);

            // 检查MQTT是否已连接
        if (mqtt_connected) {
            // 创建一个JSON对象
            cJSON *json_data = cJSON_CreateObject();  
            char temperature_str[6];
            char humidity_str[6];

            sprintf(temperature_str, "%.1f", aht20_data_t.temperature);
            sprintf(humidity_str, "%.1f", aht20_data_t.rel_humidity);      
            cJSON_AddStringToObject(json_data, "temperature", temperature_str);
            cJSON_AddStringToObject(json_data, "humidity", humidity_str);
            // cJSON_AddNumberToObject(json_data, "temperature", aht20_data_t.temperature);//未保留的值
            // cJSON_AddNumberToObject(json_data, "humidity", aht20_data_t.rel_humidity);

            // 将JSON对象转换为字符串
            char *json_string = cJSON_Print(json_data);

            // 发送JSON字符串到MQTT服务器
            int msg_id = esp_mqtt_client_publish(client, "/topic/t_h", json_string, 0, 0, 0);
            // ESP_LOGI(TAG, "aht20_published_sucsessful, msg_id=%d", msg_id);

            // 释放JSON对象和字符串
            cJSON_Delete(json_data);
            free(json_string);
        }

        vTaskDelay(3000 / portTICK_PERIOD_MS); // 等待3秒
    }

}