#ifndef RTOS_DS18B20_SENSOR_H
#define RTOS_DS18B20_SENSOR_H

#include <stdbool.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "driver/gpio.h"

// Temperature reading structure
typedef struct {
    float temperature_celsius;
    time_t timestamp_seconds;
    bool error;
    char error_message[64];
} temp_reading_t;

// DS18B20 sensor structure
typedef struct {
    gpio_num_t pin;
    temp_reading_t temperatureBuffer[10];  // BUFFER_SIZE
    int bufferIndex;
    bool readingsAvailable;
    SemaphoreHandle_t bufferMutex;
    QueueHandle_t tempQueue;
    QueueHandle_t batchQueue;
    esp_timer_handle_t temp_timer;
    esp_timer_handle_t batch_timer;
    bool initialized;
    bool queues_initialized;  // Track queue initialization separately
    int error_count;
} ds18b20_sensor_t;

// Function declarations
void start_temperature_sensor(ds18b20_sensor_t* sensor);
void storeTemperatureTask(void *pvParameters);

// External sensor instances
extern ds18b20_sensor_t sensor1;
extern ds18b20_sensor_t sensor2;

#endif // RTOS_DS18B20_SENSOR_H 