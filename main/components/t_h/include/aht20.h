#ifndef AHT_20_H
#define AHT_20_H

#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_types.h"

extern struct aht20_data{
    float temperature;
    float rel_humidity;
}aht20_data_t;

void check_calibration();
bool getbit(int NUM, int n);
void trigger_measure();
void aht20_read_measures(void *ignore);
void aht20_init(gpio_num_t clock, gpio_num_t data);
void i2c_setup();



#endif