#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "demo_header.h"

static const char *TAG = "DS18B20_AWS_MAIN";

// Forward declaration for the AWS IoT demo function
extern int aws_iot_demo_main(int argc, char **argv);

// Define temperature reading structure
typedef struct {
    float temperature_celsius;
    time_t timestamp_seconds;
} temp_reading_t;

// Declare queue for temperature readings
extern QueueHandle_t tempQueue;

// External function declarations
void start_temperature_sensor(void);
extern void storeTemperatureTask(void *pvParameters);

// Semaphore to ensure AWS IoT is initialized before starting to publish
SemaphoreHandle_t mqtt_conn_ready;

// AWS IoT task that handles MQTT connections and publishing
void aws_iot_task(void *param) {
    // Wait for network to be ready before starting AWS IoT
    while (1) {
        // Call AWS IoT demo with temperature queue as parameter
        aws_iot_demo_main(0, NULL);
        
        ESP_LOGE(TAG, "AWS IoT task ended unexpectedly. Restarting in 5 seconds...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main() {
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);

    // Initialize NVS for WiFi configuration storage
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create queue for temperature readings if not already created
    extern QueueHandle_t tempQueue;
    if (tempQueue == NULL) {
        tempQueue = xQueueCreate(10, sizeof(temp_reading_t));
    }
    
    // Create semaphore for synchronization
    mqtt_conn_ready = xSemaphoreCreateBinary();

    // Configure WiFi
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "WiFi connected, starting tasks");

    // Start temperature sensor (initializes sensor and timer)
    start_temperature_sensor();
    
    // Create task for storing temperature readings
    xTaskCreate(storeTemperatureTask, "StoreTempTask", 4096, NULL, 2, NULL);
    
    // Create AWS IoT task with higher priority
    xTaskCreate(aws_iot_task, "AWS_IoT_Task", 8192, NULL, 5, NULL);
} 