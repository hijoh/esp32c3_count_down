#ifndef GUI_TASK_H
#define GUI_TASK_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void gui_task(void *arg);
void lv_tick_task(void *arg);

#endif /* GUI_TASK_H */