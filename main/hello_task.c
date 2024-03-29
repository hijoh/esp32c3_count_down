/*
*********************************************************************************************************
*
*	模块名称 : 启动任务
*	文件名称 : hello_task.c
*	版    本 : V1.0
*	说    明 : 
*   联系方式  ：嘉友创-小步（微信：15962207161）
*
*	修改记录 :
*	版本号          日期        		作者     		说明
*	V1.0           2022-09-08 		  DOUBLE  		 正式发布
*********************************************************************************************************
*/
// #include "main.h"
#include "hello_task.h"

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

