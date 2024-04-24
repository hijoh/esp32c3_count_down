#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "encoder.h"
#include "key.h"
// #include "font.h"
#include "ui.h"
#include "../lvgl/lvgl.h"
#include "driver/gpio.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"

// volatile bool ec11_rotated = false;
static u_int8_t count = 0;
static QueueHandle_t gpioEventQueue = NULL;

// static SemaphoreHandle_t xSemaphore = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    esp_task_wdt_reset();
    // uint32_t gpio_num = (uint32_t)arg;
    uint32_t gpio_num = (uint32_t)(uintptr_t)arg;

    if(xQueueSendFromISR(gpioEventQueue, &gpio_num, NULL)==errQUEUE_FULL){
        // printf("queue_full\n");// 队列已满
    };
    // xSemaphoreGiveFromISR(xSemaphore, NULL);
}
/*
void timer_callback(void* arg)
{
    printf("Timer expired!\n");
}

void gpio_task_example(void* arg)
{
    esp_task_wdt_add(NULL);
    uint32_t io_num;
    char count_str[10]; // 用于存储右转次数的字符串

    // 创建一个定时器
    esp_timer_handle_t timer;
    esp_timer_create_args_t timer_args = {
        .callback = timer_callback, // 设置回调函数
        .name = "timeout_timer" // 设置定时器的名字
    };
    esp_timer_create(&timer_args, &timer);

    while(1) {
        esp_task_wdt_reset();
        if(xQueueReceive(gpioEventQueue, &io_num, 500 / portTICK_PERIOD_MS)) 
        {
            if (power_flag) // 开关打开
            {
                if(io_num == EC11_GPIO_SCL)
                {                                
                    if(count == 1)
                    {
                        count = 3;
                        if (counter < 9) 
                        {
                            counter++;
                        }
                        else {
                            printf("Counter: MAX\n");
                            lv_label_set_text_fmt(ui_Label2, "Counter: MAX\n");
                            continue; // 跳过打印计数和更新 OLED 的步骤
                        } 
                        printf("Counter: %d\n", counter); // 打印累加数
                        snprintf(count_str, sizeof(count_str), "%d", counter);
                        lv_label_set_text_fmt(ui_Label2, "Counter: %s", count_str); // 更新标签的文本
                    }
                    else if(count == 0)
                    {
                        count = 2;
                        
                        // 启动定时器，设置超时时间为200毫秒
                        esp_timer_start_once(timer, 200 * 1000);
                    } 
                } 
                else if(io_num == EC11_GPIO_DAT)
                {
                    if(count == 2)
                    {
                        count = 3;
                        if(counter > 0) {
                            counter--; // 累加数递减
                        }else {
                            printf("Counter: Min\n");
                            lv_label_set_text_fmt(ui_Label2, "Counter: MIN\n");
                            continue; // 跳过打印计数和更新 OLED 的步骤
                        } 
                        printf("Counter: %d\n", counter);
                        snprintf(count_str, sizeof(count_str), "%d", counter);
                        lv_label_set_text_fmt(ui_Label2, "Counter: %s", count_str); // 更新标签的文本
                    }
                    else if(count==0){
                        esp_task_wdt_reset();
                        count = 1;

                        // 启动定时器，设置超时时间为200毫秒
                        esp_timer_start_once(timer, 200 * 1000);
                    }
                }           
            }
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    vQueueDelete(gpioEventQueue);
    vTaskDelete(NULL);
}
*/
static void time_out_task(void *arg)
{
    vTaskDelay(200 / portTICK_PERIOD_MS);
    count = 0;
    // gpioEventQueue = NULL;
    xQueueReset(gpioEventQueue);
    vTaskDelete(NULL);
}

static void gpio_task_example(void* arg)
{
    esp_task_wdt_add(NULL);
    uint8_t counter = 0; // 声明计数变量
    uint32_t io_num;
    char count_str[10]; // 用于存储右转次数的字符串
    for(;;) {
        esp_task_wdt_reset();
        if(xQueueReceive(gpioEventQueue, &io_num, 0)) 
        // if(xSemaphoreTake(xSemaphore, portMAX_DELAY)) 
        {
            // printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
            if (power_flag) // 开关打开
            {esp_task_wdt_reset();
            if(io_num == EC11_GPIO_SCL)
            {                                
            if(count == 1)
                {
                    count = 3;
                    if (counter < 9) 
                    {
                        counter++;
                    }
                    else {
                        printf("Counter: MAX\n");
                        lv_label_set_text_fmt(ui_Label2, "Counter: MAX\n");
                        continue; // 跳过打印计数和更新 OLED 的步骤
                    } 
                    printf("Counter: %d\n", counter); // 打印累加数
                    snprintf(count_str, sizeof(count_str), "%d", counter);
                    lv_label_set_text_fmt(ui_Label2, "Counter: %s", count_str); // 更新标签的文本
                    xQueueReset(gpioEventQueue);
                }else if(count == 0)
                {
                    count = 2;
                    xTaskCreate(time_out_task, "time_out_task", 1024, NULL, 2, NULL);
                } 
            
            } 
            else if(io_num == EC11_GPIO_DAT)
            {
                 if(count == 2)
                {
                    count = 3;
                    if(counter > 0) {
                        counter--; // 累加数递减
                    }else {
                        printf("Counter: Min\n");
                        lv_label_set_text_fmt(ui_Label2, "Counter: MIN\n");
                        continue; // 跳过打印计数和更新 OLED 的步骤
                    } 
                    printf("Counter: %d\n", counter);
                    snprintf(count_str, sizeof(count_str), "%d", counter);
                    lv_label_set_text_fmt(ui_Label2, "Counter: %s", count_str); // 更新标签的文本
                    xQueueReset(gpioEventQueue);
                }else if(count==0){
                    count = 1;
                     // Clear the entire OLED display
                    //  esp_task_wdt_reset();
                    xTaskCreate(time_out_task, "time_out_task", 1024, NULL, 2, NULL);
                }
            }           
        }
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    vQueueDelete(gpioEventQueue);
    vTaskDelete(NULL);
}


/*
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t)(uintptr_t)arg;
    if(xQueueSendFromISR(gpioEventQueue, &gpio_num, NULL)==errQUEUE_FULL){
    };
}
static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    char count_str[12]; // Increase size to 12
    int counter = 0; // Initialize counter

    for(;;) {
        if(xQueueReceive(gpioEventQueue, &io_num, portMAX_DELAY) && power_flag) {
            if(gpio_get_level(EC11_GPIO_DAT)) {
                counter++; // Increment counter
                snprintf(count_str, sizeof(count_str), "%d", counter);
                lv_label_set_text_fmt(ui_Label2, "Counter: %s", count_str); // 更新标签的文本
                printf("+ turn\n");
            } else {
                counter--; // Decrement counter
                snprintf(count_str, sizeof(count_str), "%d", counter);
                lv_label_set_text_fmt(ui_Label2, "Counter: %s", count_str); // 更新标签的文本
                printf("- turn\n"); 
            }
            vTaskDelay(20 / portTICK_PERIOD_MS);       
        }
    }
    vQueueDelete(gpioEventQueue);
    vTaskDelete(NULL);
}
*/

void encoder_init(void)
{
    // printf("Hello EC11!\n");
    gpio_config_t gpio_10 = {
        .pin_bit_mask = 1ULL << EC11_GPIO_SCL,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_NEGEDGE,
        .pull_up_en = 1,
    };
    gpio_config_t gpio_11 = {
        .pin_bit_mask = 1ULL << EC11_GPIO_DAT,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_NEGEDGE,
        .pull_up_en = 1,
    };
    gpio_config(&gpio_10);
    gpio_config(&gpio_11);

    gpioEventQueue = xQueueCreate(1000, sizeof(uint32_t));

    gpio_install_isr_service(ESP_INTR_FLAG_SHARED);
    gpio_isr_handler_add(EC11_GPIO_SCL, gpio_isr_handler, (void *)EC11_GPIO_SCL);
    gpio_isr_handler_add(EC11_GPIO_DAT, gpio_isr_handler, (void *)EC11_GPIO_DAT);
    // xTaskCreate(gpio_task_example, "gpio_task_example", 1024*6, NULL, 25, NULL);
    xTaskCreatePinnedToCore(gpio_task_example, "time_out_task", 1024*6, NULL, 25, NULL, 1);

}
