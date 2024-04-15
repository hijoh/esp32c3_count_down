#include <stdbool.h>
#include <string.h>

#include "key.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "st7789.h"
#include "ui.h"
#include "../lvgl/lvgl.h"

// #include "font.h"


bool power_flag = false;

// TickType_t button_press_time = 0;

static const char *TAG = "power_flag";  // 定义标签

void key_init(void)
{
    // Configure LED pin as output
    esp_rom_gpio_pad_select_gpio(LED_R_IO);
    gpio_set_direction(LED_R_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_R_IO, 0); // Turn off initially

    // Configure button pin as input
    esp_rom_gpio_pad_select_gpio(KEY_IO);
    gpio_set_direction(KEY_IO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(KEY_IO, GPIO_PULLUP_ONLY); // Enable internal pull-up resistor  
}

/*
void key_read1(void)
{  
    TickType_t current_time = xTaskGetTickCount();
    TickType_t button_press_duration = current_time - button_press_time;

    if (gpio_get_level(KEY_IO) == 0) // 按键按下
    {
        if (button_press_time == 0) // 按键之前未被按下
        {
            button_press_time = current_time;
            ESP_LOGI(TAG,"button_press_time: %u", (unsigned int)button_press_time);
        }
        else if (button_press_duration >= LONG_PRESS_TIME  && button_released) // 按键按下持续2秒
        {
            // 切换开关状态标志
            power_flag = !power_flag;button_released = false;
            
            if (power_flag)
            {
                ESP_LOGI(TAG, "EC11_ON");
            //    xTaskCreate(dht11_task, "DHT11 Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &dht11_task_handle);
                
                // OLED_ShowString(12, 6, "2023-11-17", 16);
                gpio_set_level(LED_R_IO, 1);
                button_press_time = 0;
            }
            else
            {
                ESP_LOGI(TAG, "EC11_OFF");
                //  OLED_Clear();
                // OLED_ShowCHinese(0 * 18, 0, 0);
                // OLED_ShowCHinese(1 * 18, 0, 1);
                // OLED_ShowString(30, 5, "GOODBYE!", 16);
                
                gpio_set_level(LED_R_IO, 0);
                vTaskDelay(1000 / portTICK_PERIOD_MS);//延时1秒
                button_press_time = 0;
                // OLED_Clear();
            }
            button_press_time = 0; // 重置按键按下时间
            
        }
    }
    else// 按键释放
    {
        button_press_time = 0; // 重置按键按下时间
        button_released = true;
    }
}
*/
void key_read(void)
{
    static uint32_t button_press_start_time = 0;
    static bool button_pressed = false;
    static bool button_released = true; // 新增的标志

    if (gpio_get_level(KEY_IO) == 0) // 按键按下
    {
        if (!button_pressed) // 按键之前未被按下
        {
            button_pressed = true;
            // button_press_start_time = esp_log_timestamp();
            button_press_start_time = xTaskGetTickCount();
            ESP_LOGI(TAG, "Button pressed");
            lv_label_set_text_fmt(ui_Label2, "POWER ON\n");
        }
        else
        {
            uint32_t button_press_duration = xTaskGetTickCount() - button_press_start_time;
            if (button_press_duration >= LONG_PRESS_TIME && button_released) // 按键按下持续2秒
            {
                // 切换开关状态标志
                power_flag = !power_flag;
                button_released = false;

                if (power_flag)
                {
                    ESP_LOGI(TAG, "EC11_ON");
                    st7789_enable_backlight(true);
                    // xpt2046_touch_enable(true);
                    xpt2046_enable();
                    gpio_set_level(LED_R_IO, 1);
                }
                else
                {
                    ESP_LOGI(TAG, "EC11_OFF");
                    gpio_set_level(LED_R_IO, 0);
                    st7789_enable_backlight(false);
                    // xpt2046_touch_enable(false);
                    xpt2046_disable();
                    vTaskDelay(1000 / portTICK_PERIOD_MS); //延时1秒
                }
                button_press_start_time = 0; // 重置按键按下时间
            }
        }
    }
    else // 按键释放
    {
        button_pressed = false;
        button_released = true;
    }
}

void key_read_task(void *pvParameters)
{
    while (1)
    {
        key_read();
        vTaskDelay(pdMS_TO_TICKS(100)); // 等待一段时间，以允许其他任务运行
    }
}



