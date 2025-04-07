#ifndef AWS_IOT_H
#define AWS_IOT_H

#include <stdint.h>
#include "core_mqtt.h"
#include "tls_freertos.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "aws_iot_config.h"

/* Forward declaration of the batch processing task */
void processBatchedReadings(void* pvParameters);

/* Configuration structure for AWS IoT */
typedef struct {
    // Connection settings
    uint16_t mqtt_port;
    uint32_t keep_alive_seconds;
    uint32_t process_loop_timeout_ms;
    uint32_t transport_timeout_ms;
    
    // Retry settings
    uint32_t max_retry_attempts;
    uint32_t max_backoff_delay_ms;
    uint32_t base_backoff_delay_ms;
    uint32_t connack_recv_timeout_ms;
    
    // Task settings
    uint32_t task_stack_size;
    uint8_t task_priority;
    
    // Buffer settings
    uint32_t network_buffer_size;
} AwsIoTConfig_t;

/* Main context structure for AWS IoT */
typedef struct {
    MQTTContext_t mqtt_context;
    NetworkContext_t network_context;
    AwsIoTConfig_t config;
    bool is_initialized;
    TaskHandle_t batch_task_handle;
    uint8_t network_buffer[NETWORK_BUFFER_SIZE];
    
    // Multiple queue support
    QueueHandle_t* batch_queues;
    size_t num_queues;
} AwsIoTContext_t;

/* Function declarations */
AwsIoTContext_t* aws_iot_setup(const AwsIoTConfig_t* config, QueueHandle_t* queues, size_t num_queues);
void aws_iot_cleanup(AwsIoTContext_t* ctx);
int aws_iot_try_reconnect(AwsIoTContext_t* ctx);
void aws_iot_task(void* param);
int publishReading(AwsIoTContext_t* ctx, const temp_reading_t* reading, int queue_index);

#endif /* AWS_IOT_H */ 