#ifndef AWS_IOT_CONFIG_H
#define AWS_IOT_CONFIG_H

#include <stdint.h>

/* AWS IoT endpoint configuration */
#define AWS_IOT_ENDPOINT               CONFIG_MQTT_BROKER_ENDPOINT
#define AWS_IOT_ENDPOINT_LENGTH        ( ( uint16_t ) ( sizeof( AWS_IOT_ENDPOINT ) - 1 ) )

/* MQTT configuration */
#define AWS_MQTT_PORT                  ( CONFIG_MQTT_BROKER_PORT )
#define MQTT_KEEP_ALIVE_INTERVAL_SECONDS    ( 60U )
#define MQTT_PROCESS_LOOP_TIMEOUT_MS        ( 1500U )
#define TRANSPORT_SEND_RECV_TIMEOUT_MS      ( 1500U )

/* Retry configuration */
#define CONNECTION_RETRY_MAX_ATTEMPTS            ( 5U )
#define CONNECTION_RETRY_MAX_BACKOFF_DELAY_MS    ( 5000U )
#define CONNECTION_RETRY_BACKOFF_BASE_MS         ( 500U )
#define CONNACK_RECV_TIMEOUT_MS                  ( 1000U )

/* Topic configuration */
#define MQTT_THERMOCOUPLE_21_TOPIC              "thermocouple/21/readings"
#define MQTT_THERMOCOUPLE_21_TOPIC_LENGTH       ( ( uint16_t ) ( sizeof( MQTT_THERMOCOUPLE_21_TOPIC ) - 1 ) )

#define MQTT_THERMOCOUPLE_22_TOPIC              "thermocouple/22/readings"
#define MQTT_THERMOCOUPLE_22_TOPIC_LENGTH       ( ( uint16_t ) ( sizeof( MQTT_THERMOCOUPLE_22_TOPIC ) - 1 ) )

/* Buffer configuration */
#define NETWORK_BUFFER_SIZE                ( CONFIG_MQTT_NETWORK_BUFFER_SIZE )

/* Task configuration */
#define AWS_IOT_TASK_STACK_SIZE            ( 8192 )
#define AWS_IOT_TASK_PRIORITY              ( 5 )

/* Client identifier */
#define CLIENT_IDENTIFIER                   "esp32_ds18b20"
#define CLIENT_IDENTIFIER_LENGTH            ( ( uint16_t ) ( sizeof( CLIENT_IDENTIFIER ) - 1 ) )

/* Metrics string */
#define METRICS_STRING                      "?SDK=ESP-IDF&Version=4.4"
#define METRICS_STRING_LENGTH               ( ( uint16_t ) ( sizeof( METRICS_STRING ) - 1 ) )

#endif /* AWS_IOT_CONFIG_H */ 