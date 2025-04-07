#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "aws_iot.h"
#include "rtos_ds18b20_sensor.h"
#include "aws_iot_config.h"

static const char *TAG = "APP_MAIN";

// AWS IoT configuration
static AwsIoTConfig_t aws_iot_config = {
    .mqtt_port = AWS_MQTT_PORT,
    .keep_alive_seconds = MQTT_KEEP_ALIVE_INTERVAL_SECONDS,
    .process_loop_timeout_ms = MQTT_PROCESS_LOOP_TIMEOUT_MS,
    .transport_timeout_ms = TRANSPORT_SEND_RECV_TIMEOUT_MS,
    .max_retry_attempts = CONNECTION_RETRY_MAX_ATTEMPTS,
    .max_backoff_delay_ms = CONNECTION_RETRY_MAX_BACKOFF_DELAY_MS,
    .base_backoff_delay_ms = CONNECTION_RETRY_BACKOFF_BASE_MS,
    .connack_recv_timeout_ms = CONNACK_RECV_TIMEOUT_MS,
    .task_stack_size = AWS_IOT_TASK_STACK_SIZE,
    .task_priority = AWS_IOT_TASK_PRIORITY,
    .network_buffer_size = NETWORK_BUFFER_SIZE
};

// Temperature sensor contexts
ds18b20_sensor_t sensor1;
ds18b20_sensor_t sensor2;

// AWS IoT context
static AwsIoTContext_t* aws_iot_ctx = NULL;

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP32 DS18B20 Temperature Sensor with AWS IoT");
    ESP_LOGI(TAG, "Free heap: %d", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    // Initialize sensors first (this creates the queues)
    start_temperature_sensor(&sensor1);
    start_temperature_sensor(&sensor2);

    // Verify queues were created successfully
    if (!sensor1.queues_initialized || !sensor2.queues_initialized) {
        ESP_LOGE(TAG, "Failed to initialize sensor queues");
        return;
    }

    // Create AWS IoT context
    QueueHandle_t queues[] = { sensor1.batchQueue, sensor2.batchQueue };
    aws_iot_ctx = aws_iot_setup(&aws_iot_config, queues, 2);
    if (aws_iot_ctx == NULL) {
        ESP_LOGE(TAG, "Failed to initialize AWS IoT context");
        return;
    }

    // Create AWS IoT task
    BaseType_t xReturned = xTaskCreate(aws_iot_task,
                                     "aws_iot_task",
                                     aws_iot_config.task_stack_size,
                                     aws_iot_ctx,
                                     aws_iot_config.task_priority,
                                     NULL);
    if (xReturned != pdPASS) {
        ESP_LOGE(TAG, "Failed to create AWS IoT task");
        aws_iot_cleanup(aws_iot_ctx);
        return;
    }

    ESP_LOGI(TAG, "Application initialized successfully");
} 