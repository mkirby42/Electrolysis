#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/adc.h" // replace with esp_adc/adc_oneshot.h

#define TEMP_READ_INTERVAL_MS 2000  // Read temperature every 2 seconds
#define BUFFER_SIZE 10               // Store last 10 readings

static const char *TAG = "TemperatureApp";

// Circular buffer to store temperature readings
float temperatureBuffer[BUFFER_SIZE];
int bufferIndex = 0;

// FreeRTOS Queue for inter-task communication
QueueHandle_t tempQueue;

// Simulated function to read ESP32 internal temperature sensor (ADC method)
float readInternalTemperature() {
    int raw_adc;
    adc1_config_width(ADC_WIDTH_BIT_12);          // Set 12-bit resolution
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11); // Use channel 0
    raw_adc = adc1_get_raw(ADC1_CHANNEL_0);       // Get raw ADC reading

    // Convert raw ADC value to temperature (rough approximation)
    float temperature = (raw_adc / 4096.0) * 100.0; // Arbitrary scaling for example
    return temperature;
}

// Task 1: Reads the temperature and sends it to the queue
void readTemperatureTask(void *pvParameters) {
    while (1) {
        float temperature = readInternalTemperature();
        ESP_LOGI(TAG, "Temperature Read: %.2f°C", temperature);

        // Send temperature to the queue
        xQueueSend(tempQueue, &temperature, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(TEMP_READ_INTERVAL_MS));
    }
}

// Task 2: Stores temperature readings in a circular buffer
void storeTemperatureTask(void *pvParameters) {
    float receivedTemp;
    
    while (1) {
        // Wait for a new temperature reading from the queue
        if (xQueueReceive(tempQueue, &receivedTemp, portMAX_DELAY)) {
            temperatureBuffer[bufferIndex] = receivedTemp;
            bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;

            ESP_LOGI(TAG, "Stored Temperature: %.2f°C (Index: %d)", receivedTemp, bufferIndex);
        }
    }
}

// Main function to initialize FreeRTOS tasks
void app_main() {
    ESP_LOGI(TAG, "Starting FreeRTOS Temperature Monitoring...");

    // Create a queue to store temperature readings
    tempQueue = xQueueCreate(5, sizeof(float)); // Queue can hold 5 temperature readings

    // Create FreeRTOS tasks
    xTaskCreate(readTemperatureTask, "ReadTempTask", 2048, NULL, 2, NULL);
    xTaskCreate(storeTemperatureTask, "StoreTempTask", 2048, NULL, 1, NULL);
}
