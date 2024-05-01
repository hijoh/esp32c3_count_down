
// #include "main.h"
#include "hello_task.h"
// #include "ui.h"
// #include "../lvgl/lvgl.h"
#include "mqtt.h"

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
    //初始化IO
    bsp_gpio_init();
    //启动mqtt
    mqtt_start();
    display_target_rec();
    machine_status();
    while (1) 
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
