#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "core_mqtt.h"
#include "demo_config.h"
#include "demo_header.h"
#include "cJSON.h"

static const char *TAG = "MQTT_BATCH";

// Define the buffer size and AWS topic for temperature data
#define BUFFER_SIZE 10  // Maximum number of readings in a batch
#define TEMPERATURE_TOPIC "device/temperature/batch"
#define TEMPERATURE_TOPIC_LENGTH sizeof(TEMPERATURE_TOPIC) - 1

// Forward declaration of the publishing function from mqtt_demo_mutual_auth.c
extern MQTTStatus_t publishMQTTMessage(MQTTContext_t *pMqttContext, const char *topic, 
                                uint16_t topicLen, const char *payload, size_t payloadLen);

// Task that processes batched temperature readings from queue
void processBatchedReadings(void *pvParameters)
{
    MQTTContext_t *pMqttContext = (MQTTContext_t *)pvParameters;
    int readingsCount = 0;
    temp_reading_t readings[BUFFER_SIZE];
    char *jsonBuffer = NULL;
    
    ESP_LOGI(TAG, "Batch processing task started");
    
    while (1) {
        // First dequeue the count of readings in this batch
        if (xQueueReceive(batchQueue, &readingsCount, portMAX_DELAY)) {
            
            ESP_LOGI(TAG, "Processing batch of %d readings", readingsCount);
            
            // Then dequeue each reading
            for (int i = 0; i < readingsCount; i++) {
                if (xQueueReceive(batchQueue, &readings[i], portMAX_DELAY) != pdTRUE) {
                    ESP_LOGE(TAG, "Failed to receive reading %d from queue", i);
                    readingsCount = i; // Adjust count to what we actually got
                    break;
                }
            }
            
            // Create a JSON document with all readings
            cJSON *root = cJSON_CreateObject();
            cJSON *readingsArray = cJSON_CreateArray();
            
            // Add device info
            cJSON_AddStringToObject(root, "device_id", CLIENT_IDENTIFIER);
            cJSON_AddItemToObject(root, "readings", readingsArray);
            
            // Add each reading to the array
            for (int i = 0; i < readingsCount; i++) {
                cJSON *reading = cJSON_CreateObject();
                cJSON_AddNumberToObject(reading, "temp_c", readings[i].temperature_celsius);
                cJSON_AddNumberToObject(reading, "temp_f", (readings[i].temperature_celsius * 9.0 / 5.0) + 32.0);
                cJSON_AddNumberToObject(reading, "timestamp", (double)readings[i].timestamp_seconds);
                cJSON_AddItemToArray(readingsArray, reading);
            }
            
            // Convert to string
            jsonBuffer = cJSON_PrintUnformatted(root);
            
            // Publish to AWS IoT if MQTT is connected
            if (jsonBuffer != NULL && pMqttContext != NULL) {
                ESP_LOGI(TAG, "Publishing batch: %s", jsonBuffer);
                
                MQTTStatus_t mqttStatus = publishMQTTMessage(
                    pMqttContext,
                    TEMPERATURE_TOPIC,
                    TEMPERATURE_TOPIC_LENGTH, 
                    jsonBuffer,
                    strlen(jsonBuffer)
                );
                
                if (mqttStatus != MQTTSuccess) {
                    ESP_LOGE(TAG, "Failed to publish batch: %d", mqttStatus);
                } else {
                    ESP_LOGI(TAG, "Successfully published batch of %d readings", readingsCount);
                }
                
                // Free the JSON string
                free(jsonBuffer);
            } else {
                ESP_LOGE(TAG, "Failed to create JSON or MQTT not ready, context: %p", pMqttContext);
            }
            
            // Free the JSON object
            cJSON_Delete(root);
        }
        
        // Short delay to prevent 100% CPU usage if queue is empty
        vTaskDelay(pdMS_TO_TICKS(100));
    }
} 