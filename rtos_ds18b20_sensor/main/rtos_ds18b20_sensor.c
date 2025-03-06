#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "esp_timer.h"
#include "hal/gpio_types.h"
#include "esp_system.h"
#include "time.h"

#define TEMP_READ_INTERVAL_MS 2000  // Read temperature every 2 seconds
#define BUFFER_SIZE 10              // Store last 10 readings
#define DS18B20_PIN GPIO_NUM_4      // DS18B20 data pin
#define SKIP_ROM 0xCC
#define CONVERT_T 0x44
#define READ_SCRATCHPAD 0xBE

static const char *TAG = "DS18B20App";

// Add this struct after your existing defines
typedef struct {
    float temperature_celsius;
    time_t timestamp_seconds;
} temp_reading_t;

// Update the buffer and queue to use the new struct
temp_reading_t temperatureBuffer[BUFFER_SIZE];
int bufferIndex = 0;

// FreeRTOS Queue for inter-task communication
QueueHandle_t tempQueue;

// Timer handle
esp_timer_handle_t temp_timer;

// OneWire protocol functions
static void ds18b20_init(void) {
    gpio_set_direction(DS18B20_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DS18B20_PIN, 1);
}

static bool ds18b20_reset(void) {
    gpio_set_direction(DS18B20_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DS18B20_PIN, 0);
    ets_delay_us(480);
    gpio_set_direction(DS18B20_PIN, GPIO_MODE_INPUT);
    ets_delay_us(70);
    bool presence = !gpio_get_level(DS18B20_PIN);
    ets_delay_us(410);
    return presence;
}

static void ds18b20_write_bit(bool bit) {
    gpio_set_direction(DS18B20_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DS18B20_PIN, 0);
    ets_delay_us(bit ? 1 : 60);
    gpio_set_level(DS18B20_PIN, 1);
    ets_delay_us(bit ? 60 : 1);
}

static bool ds18b20_read_bit(void) {
    bool bit;
    gpio_set_direction(DS18B20_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DS18B20_PIN, 0);
    ets_delay_us(2);
    gpio_set_direction(DS18B20_PIN, GPIO_MODE_INPUT);
    ets_delay_us(10);
    bit = gpio_get_level(DS18B20_PIN);
    ets_delay_us(50);
    return bit;
}

static void ds18b20_write_byte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        ds18b20_write_bit(data & (1 << i));
    }
}

static uint8_t ds18b20_read_byte(void) {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        if (ds18b20_read_bit()) {
            data |= (1 << i);
        }
    }
    return data;
}

float readDS18B20Temperature() {
    if (!ds18b20_reset()) {
        ESP_LOGE(TAG, "DS18B20 not detected!");
        return -999.0;
    }

    ds18b20_write_byte(SKIP_ROM);
    ds18b20_write_byte(CONVERT_T);
    vTaskDelay(pdMS_TO_TICKS(750)); // Wait for conversion

    ds18b20_reset();
    ds18b20_write_byte(SKIP_ROM);
    ds18b20_write_byte(READ_SCRATCHPAD);

    uint8_t temp_lsb = ds18b20_read_byte();
    uint8_t temp_msb = ds18b20_read_byte();

    int16_t raw = (temp_msb << 8) | temp_lsb;
    float temperature = raw / 16.0;

    return temperature;
}

// Timer callback function
static void timer_callback(void* arg) {
    temp_reading_t reading;
    reading.temperature_celsius = readDS18B20Temperature();
    reading.timestamp_seconds = time(NULL);
    
    if (reading.temperature_celsius > -999.0) {
        float temperatureF = (reading.temperature_celsius * 9 / 5) + 32;
        ESP_LOGI(TAG, "Temperature Read: %.2f°C / %.2f°F at %ld", 
                 reading.temperature_celsius, temperatureF, reading.timestamp_seconds);
        xQueueSend(tempQueue, &reading, portMAX_DELAY);
    }
}

// Store temperature readings in circular buffer
void storeTemperatureTask(void *pvParameters) {
    temp_reading_t receivedReading;
    UBaseType_t watermark;
    
    while (1) {
        if (xQueueReceive(tempQueue, &receivedReading, portMAX_DELAY)) {
            temperatureBuffer[bufferIndex] = receivedReading;
            bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
            
            // Convert timestamp to readable format
            char timeStr[64];
            struct tm timeinfo;
            localtime_r(&receivedReading.timestamp_seconds, &timeinfo);
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
            
            ESP_LOGI(TAG, "Stored Temperature: %.2f°C at %s (Index: %d)", 
                     receivedReading.temperature_celsius, timeStr, bufferIndex);
        }

        // Print remaining stack space periodically
        watermark = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGI(TAG, "Remaining stack: %d bytes", watermark * 4);
    }
}

void app_main() {
    ESP_LOGI(TAG, "Starting DS18B20 Temperature Monitoring...");

    // Initialize DS18B20
    ds18b20_init();

    // Create queue
    tempQueue = xQueueCreate(5, sizeof(temp_reading_t));

    // Configure timer
    esp_timer_create_args_t timer_args = {
        .callback = timer_callback,
        .name = "temp_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &temp_timer));

    // Start timer
    ESP_ERROR_CHECK(esp_timer_start_periodic(temp_timer, TEMP_READ_INTERVAL_MS * 1000));

    // Create storage task
    xTaskCreate(storeTemperatureTask, "StoreTempTask", 2048, NULL, 1, NULL);
} 