/*
*********************************************************************************************************
*
*	模块名称 : IO初始化
*	文件名称 : bsp_gpio.c
*	版    本 : V1.0
*	说    明 : 
*
*	修改记录 :
*	版本号          日期        		作者     		说明
*	V1.0           2022-09-08 		  DOUBLE  		 正式发布
*********************************************************************************************************
*/
#include "bsp_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bsp_gpio_led_status";
/*
*********************************************************************************************************
*	函 数 名: bsp_gpio_init
*	功能说明: IO初始化
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_gpio_init(void)
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT); 
}
/*
*********************************************************************************************************
*	函 数 名: led_on
*	功能说明: 开灯
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void led_on(void)
{
   // ESP_LOGI(TAG, "led on");
   gpio_set_level(BLINK_GPIO, 1);
}
/*
*********************************************************************************************************
*	函 数 名: led_off
*	功能说明: 关灯
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void led_off(void)
{
   // ESP_LOGI(TAG, "led off or MQTT disconnected");
   gpio_set_level(BLINK_GPIO, 0);
}

void led_status(void)
{
   // ESP_LOGI(TAG, "led status or MQTT published sucssfull");
   // for (int i = 0; i < 3; i++)
   // // while (1)
   // {        
   gpio_set_level(BLINK_GPIO, 1);
   //  ESP_LOGI(TAG, "led on");
   vTaskDelay(pdMS_TO_TICKS(100));
   gpio_set_level(BLINK_GPIO, 0);
   //  ESP_LOGI(TAG, "led off");
   vTaskDelay(pdMS_TO_TICKS(100));
   // }


}
