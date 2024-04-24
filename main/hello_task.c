
// #include "main.h"
#include "hello_task.h"
#include "ui.h"
#include "../lvgl/lvgl.h"

#define STORAGE_NAMESPACE "storage"

static const char *TAG = "helloTask";

EventGroupHandle_t hello_task_event_group;
/*
*********************************************************************************************************
*	函 数 名: hello_task
*	功能说明: 应用任务
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void hello_task(void * parm)
{
    EventBits_t uxBits;
    // //等待事件产生
    // uxBits = xEventGroupWaitBits(hello_task_event_group, 
    //                             WIFI_CONNECTED_BIT | WIFI_FAIL_BIT | WIFI_SMARTCONFIG_BIT, 
    //                             true, false, portMAX_DELAY);
    // if (uxBits & WIFI_CONNECTED_BIT) {
    //     ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
    //              _user_wifi_data.ssid, _user_wifi_data.pw);
    // } else if (uxBits & WIFI_FAIL_BIT) {
    //     ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
    //              _user_wifi_data.ssid, _user_wifi_data.pw);
    // } else if (uxBits & WIFI_SMARTCONFIG_BIT) {
    //     ESP_LOGI(TAG, "smartConfig over start app");
    // } else {
    //     ESP_LOGE(TAG, "UNEXPECTED EVENT");
    // }

    //初始化IO
    bsp_gpio_init();
    //启动mqtt
    mqtt_start();
    while (1) 
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
    }
}

void timer_task(void *pvParameter)
{
    nvs_handle_t my_handle;
    esp_err_t err;
    uint32_t sec = 0;
    uint32_t sec1 = 0;
    uint32_t sec2 = 0;
    // uint32_t t202 = 0;

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        // Initialize sec to 0
        // err = nvs_set_u32(my_handle, "sec", 0);
        // if (err != ESP_OK) {
        //     printf("Error (%s) setting NVS handle!\n", esp_err_to_name(err));
        // }

        // Read
        err = nvs_get_u32(my_handle, "sec", &sec);
        switch (err) {
            case ESP_OK:
                printf("sec = %ld\n", sec);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }

        // Update sec every second
        while (1) {
            sec++;
            sec1 = sec / 60;
            sec2 = sec / 3600;
            // if (sec % 60 == 0) {
            //     sec1++;
            // }
            err = nvs_set_u32(my_handle, "sec", sec);
            printf("sec = %ld\n", sec);
            lv_label_set_text_fmt(ui_Label_t1, "sec: %ld", sec); // 更新标签的文本
            lv_label_set_text_fmt(ui_Label_t2, "min: %ld", sec1); // 更新标签的文本
            lv_label_set_text_fmt(ui_Label_t3, "hour: %ld", sec2); // 更新标签的文本
            nvs_commit(my_handle);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}