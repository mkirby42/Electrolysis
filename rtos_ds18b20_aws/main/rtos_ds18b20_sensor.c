#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "esp_timer.h"
#include "hal/gpio_types.h"
#include "esp_system.h"
#include "freertos/semphr.h"
#include "rtos_ds18b20_sensor.h"

// Define temperature reading interval
#define TEMP_READ_INTERVAL_MS 5000   // Read temperature every 5 seconds
#define BATCH_UPLOAD_INTERVAL_MS 30000  // Upload batch every 30 seconds
#define BUFFER_SIZE 10              // Store last 10 readings
#define SKIP_ROM 0xCC
#define CONVERT_T 0x44
#define READ_SCRATCHPAD 0xBE

static const char *TAG = "DS18B20Sensor";

// Global sensor instances
ds18b20_sensor_t sensor1 = {
    .pin = GPIO_NUM_21,
    .bufferIndex = 0,
    .readingsAvailable = false,
    .initialized = false,
    .queues_initialized = false,
    .error_count = 0
};

ds18b20_sensor_t sensor2 = {
    .pin = GPIO_NUM_22,
    .bufferIndex = 0,
    .readingsAvailable = false,
    .initialized = false,
    .queues_initialized = false,
    .error_count = 0
};

// OneWire protocol functions
static int ds18b20_init(ds18b20_sensor_t* sensor) {
    if (sensor == NULL) {
        ESP_LOGE(TAG, "Invalid sensor pointer");
        return EXIT_FAILURE;
    }

    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << sensor->pin),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Set GPIO level
    
    gpio_set_direction(sensor->pin, GPIO_MODE_OUTPUT);
    gpio_set_level(sensor->pin, 1);
    
    // Only create mutex if it doesn't exist yet
    if (sensor->bufferMutex == NULL) {
        sensor->bufferMutex = xSemaphoreCreateMutex();
        if (sensor->bufferMutex == NULL) {
            ESP_LOGE(TAG, "Failed to create mutex");
            return;
        }
    }
    
    sensor->initialized = true;
}

static bool ds18b20_reset(ds18b20_sensor_t* sensor) {
    if (sensor == NULL || !sensor->initialized) {
        ESP_LOGE(TAG, "Invalid or uninitialized sensor");
        return false;
    }

    if (xSemaphoreTake(sensor->bufferMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return false;
    }

    gpio_set_direction(sensor->pin, GPIO_MODE_OUTPUT);
    gpio_set_level(sensor->pin, 0);
    ets_delay_us(480);
    gpio_set_direction(sensor->pin, GPIO_MODE_INPUT);
    ets_delay_us(70);
    bool presence = !gpio_get_level(sensor->pin);
    ets_delay_us(410);

    xSemaphoreGive(sensor->bufferMutex);
    return presence;
}

static void ds18b20_write_bit(ds18b20_sensor_t* sensor, bool bit) {
    gpio_set_direction(sensor->pin, GPIO_MODE_OUTPUT);
    gpio_set_level(sensor->pin, 0);
    ets_delay_us(bit ? 1 : 60);
    gpio_set_level(sensor->pin, 1);
    ets_delay_us(bit ? 60 : 1);
}

static bool ds18b20_read_bit(ds18b20_sensor_t* sensor) {
    bool bit;
    gpio_set_direction(sensor->pin, GPIO_MODE_OUTPUT);
    gpio_set_level(sensor->pin, 0);
    ets_delay_us(2);
    gpio_set_direction(sensor->pin, GPIO_MODE_INPUT);
    ets_delay_us(10);
    bit = gpio_get_level(sensor->pin);
    ets_delay_us(50);
    return bit;
}

static void ds18b20_write_byte(ds18b20_sensor_t* sensor, uint8_t data) {
    for (int i = 0; i < 8; i++) {
        ds18b20_write_bit(sensor, data & (1 << i));
    }
}

static uint8_t ds18b20_read_byte(ds18b20_sensor_t* sensor) {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        if (ds18b20_read_bit(sensor)) {
            data |= (1 << i);
        }
    }
    return data;
}

float readDS18B20Temperature(ds18b20_sensor_t* sensor) {
    if (sensor == NULL || !sensor->initialized) {
        ESP_LOGE(TAG, "Invalid or uninitialized sensor");
        return -999.0;
    }

    if (xSemaphoreTake(sensor->bufferMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return -999.0;
    }

    bool success = false;
    float temperature = -999.0;
    int retry_count = 0;
    const int max_retries = 3;

    while (!success && retry_count < max_retries) {
        if (ds18b20_reset(sensor)) {
            ds18b20_write_byte(sensor, SKIP_ROM);
            ds18b20_write_byte(sensor, CONVERT_T);
            vTaskDelay(pdMS_TO_TICKS(750)); // Wait for conversion

            if (ds18b20_reset(sensor)) {
                ds18b20_write_byte(sensor, SKIP_ROM);
                ds18b20_write_byte(sensor, READ_SCRATCHPAD);

                uint8_t temp_lsb = ds18b20_read_byte(sensor);
                uint8_t temp_msb = ds18b20_read_byte(sensor);

                int16_t raw = (temp_msb << 8) | temp_lsb;
                temperature = raw / 16.0;
                
                // Validate temperature reading
                if (temperature > -50.0 && temperature < 125.0) {
                    success = true;
                } else {
                    ESP_LOGW(TAG, "Invalid temperature reading: %.2f°C", temperature);
                }
            }
        }
        
        if (!success) {
            retry_count++;
            ESP_LOGW(TAG, "Temperature read failed, retry %d/%d", retry_count, max_retries);
            vTaskDelay(pdMS_TO_TICKS(100)); // Short delay between retries
        }
    }

    xSemaphoreGive(sensor->bufferMutex);
    
    if (!success) {
        ESP_LOGE(TAG, "Failed to read temperature after %d attempts", max_retries);
        sensor->error_count++;
    }
    
    return temperature;
}

// Timer callback function
static void timer_callback(void* arg) {
    ds18b20_sensor_t* sensor = (ds18b20_sensor_t*)arg;
    if (sensor == NULL || !sensor->initialized) {
        ESP_LOGE(TAG, "Invalid or uninitialized sensor in timer callback");
        return;
    }
    
    temp_reading_t reading;
    reading.temperature_celsius = readDS18B20Temperature(sensor);
    reading.timestamp_seconds = time(NULL);
    reading.error = (reading.temperature_celsius <= -999.0);
    
    if (reading.error) {
        snprintf(reading.error_message, sizeof(reading.error_message), 
                "Failed to read temperature after %d attempts", 3);
    } else {
        reading.error_message[0] = '\0';
    }
    
    if (reading.temperature_celsius > -999.0) {
        float temperatureF = (reading.temperature_celsius * 9 / 5) + 32;
        ESP_LOGI(TAG, "Temperature Read: %.2f°C / %.2f°F at %ld (queued for batching)", 
                 reading.temperature_celsius, temperatureF, reading.timestamp_seconds);
        
        // Send to local storage queue
        if (sensor->tempQueue != NULL) {
            if (xQueueSend(sensor->tempQueue, &reading, 0) != pdTRUE) {
                ESP_LOGE(TAG, "Failed to send reading to temp queue");
            } else {
                // Set flag that readings are available for batch
                sensor->readingsAvailable = true;
            }
        } else {
            ESP_LOGE(TAG, "Temp queue is NULL");
        }
    }
}

// Batch timer callback - triggers sending collected readings to AWS
static void batch_timer_callback(void* arg) {
    ds18b20_sensor_t* sensor = (ds18b20_sensor_t*)arg;
    if (sensor == NULL || !sensor->initialized) {
        ESP_LOGE(TAG, "Invalid or uninitialized sensor in batch timer callback");
        return;
    }
    
    ESP_LOGI(TAG, "Batch timer fired - preparing readings for upload");
    
    if (sensor->readingsAvailable) {
        // Create a copy of the buffer to send
        temp_reading_t batchReadings[BUFFER_SIZE];
        int readingsCount = 0;
        
        // Use mutex to protect buffer access
        if (xSemaphoreTake(sensor->bufferMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Copy readings from the circular buffer
            for (int i = 0; i < BUFFER_SIZE; i++) {
                // Only include valid readings (non-zero timestamp)
                if (sensor->temperatureBuffer[i].timestamp_seconds > 0) {
                    batchReadings[readingsCount] = sensor->temperatureBuffer[i];
                    readingsCount++;
                }
            }
            
            xSemaphoreGive(sensor->bufferMutex);
        } else {
            ESP_LOGW(TAG, "Could not acquire mutex for batch processing");
            return;
        }
        
        if (readingsCount > 0) {
            ESP_LOGI(TAG, "Sending batch of %d readings to AWS", readingsCount);
            
            // Send batch to AWS IoT publishing queue
            // We'll send the buffer and count to be processed by the MQTT task
            if (sensor->batchQueue != NULL) {
                if (xQueueSend(sensor->batchQueue, &readingsCount, 0) == pdTRUE) {
                    // Send the readings themselves
                    bool allSent = true;
                    for (int i = 0; i < readingsCount; i++) {
                        if (xQueueSend(sensor->batchQueue, &batchReadings[i], 0) != pdTRUE) {
                            ESP_LOGE(TAG, "Failed to send reading %d to batch queue", i);
                            allSent = false;
                            break;
                        }
                    }
                    
                    if (allSent) {
                        ESP_LOGI(TAG, "Batch queued for upload");
                    } else {
                        ESP_LOGE(TAG, "Failed to send all readings to batch queue");
                    }
                } else {
                    ESP_LOGE(TAG, "Failed to send batch count to queue");
                }
            } else {
                ESP_LOGE(TAG, "Batch queue is NULL");
            }
        } else {
            ESP_LOGI(TAG, "No readings to upload in this batch");
        }
    } else {
        ESP_LOGI(TAG, "No new readings since last batch upload");
    }
    
    // Reset flag for next batch period
    sensor->readingsAvailable = false;
}

// Store temperature readings in circular buffer
void storeTemperatureTask(void *pvParameters) {
    ds18b20_sensor_t* sensor = (ds18b20_sensor_t*)pvParameters;
    temp_reading_t receivedReading;
    UBaseType_t watermark;
    
    while (1) {
        if (xQueueReceive(sensor->tempQueue, &receivedReading, portMAX_DELAY)) {
            // Use mutex for buffer access
            if (xSemaphoreTake(sensor->bufferMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                sensor->temperatureBuffer[sensor->bufferIndex] = receivedReading;
                sensor->bufferIndex = (sensor->bufferIndex + 1) % BUFFER_SIZE;
                
                xSemaphoreGive(sensor->bufferMutex);
                
                // Convert timestamp to readable format
                char timeStr[64];
                struct tm timeinfo;
                localtime_r(&receivedReading.timestamp_seconds, &timeinfo);
                strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
                
                ESP_LOGI(TAG, "Stored Temperature: %.2f°C at %s (Index: %d)", 
                         receivedReading.temperature_celsius, timeStr, sensor->bufferIndex);
            } else {
                ESP_LOGW(TAG, "Could not acquire mutex for storing temperature");
            }
        }

        // Print remaining stack space periodically
        watermark = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGI(TAG, "Remaining stack: %d bytes", watermark * 4);
        
        // Add a small delay to prevent high CPU usage if queue is empty
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Start temperature reading timer
int start_temperature_sensor(ds18b20_sensor_t* sensor) {
    if (sensor == NULL) {
        ESP_LOGE(TAG, "Invalid sensor pointer");
        return EXIT_FAILURE;
    }

    // Initialize the sensor first
    if (ds18b20_init(sensor) != EXIT_SUCCESS) {
        ESP_LOGE(TAG, "Failed to initialize sensor");
        return EXIT_FAILURE;
    }

    // Create queues only after successful initialization
    sensor->tempQueue = xQueueCreate(QUEUE_SIZE, sizeof(temp_reading_t));
    if (sensor->tempQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create temperature queue");
        return EXIT_FAILURE;
    }

    sensor->batchQueue = xQueueCreate(QUEUE_SIZE, sizeof(temp_reading_t));
    if (sensor->batchQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create batch queue");
        vQueueDelete(sensor->tempQueue);
        return EXIT_FAILURE;
    }

    sensor->queues_initialized = true;

    // Create timers only after queues are initialized
    esp_timer_create_args_t timer_args = {
        .callback = &timer_callback,
        .arg = sensor,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "temp_timer"
    };

    if (esp_timer_create(&timer_args, &sensor->tempTimer) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create temperature timer");
        vQueueDelete(sensor->tempQueue);
        vQueueDelete(sensor->batchQueue);
        return EXIT_FAILURE;
    }

    esp_timer_create_args_t batch_timer_args = {
        .callback = &batch_timer_callback,
        .arg = sensor,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "batch_timer"
    };

    if (esp_timer_create(&batch_timer_args, &sensor->batchTimer) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create batch timer");
        esp_timer_delete(sensor->tempTimer);
        vQueueDelete(sensor->tempQueue);
        vQueueDelete(sensor->batchQueue);
        return EXIT_FAILURE;
    }

    // Start timers only after successful creation
    if (esp_timer_start_periodic(sensor->tempTimer, TEMP_READ_INTERVAL_MS * 1000) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start temperature timer");
        esp_timer_delete(sensor->tempTimer);
        esp_timer_delete(sensor->batchTimer);
        vQueueDelete(sensor->tempQueue);
        vQueueDelete(sensor->batchQueue);
        return EXIT_FAILURE;
    }

    if (esp_timer_start_periodic(sensor->batchTimer, BATCH_UPLOAD_INTERVAL_MS * 1000) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start batch timer");
        esp_timer_stop(sensor->tempTimer);
        esp_timer_delete(sensor->tempTimer);
        esp_timer_delete(sensor->batchTimer);
        vQueueDelete(sensor->tempQueue);
        vQueueDelete(sensor->batchQueue);
        return EXIT_FAILURE;
    }

    ESP_LOGI(TAG, "Temperature sensor started successfully");
    return EXIT_SUCCESS;
} 