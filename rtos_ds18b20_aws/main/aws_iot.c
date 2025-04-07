#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "aws_iot.h"
#include "demo_config.h"
#include "backoff_algorithm.h"
#include "clock.h"
#include "rtos_ds18b20_sensor.h"

#ifndef ROOT_CA_PEM
    #if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
    static const uint8_t root_cert_auth_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
    #else
    extern const uint8_t root_cert_auth_pem_start[]   asm("_binary_root_cert_auth_pem_start");
    #endif
    extern const uint8_t root_cert_auth_pem_end[]   asm("_binary_root_cert_auth_pem_end");
#endif

#ifndef CLIENT_IDENTIFIER
    #error "Please define a unique client identifier, CLIENT_IDENTIFIER, in demo_config.h."
#endif

/* The AWS IoT message broker requires either a set of client certificate/private key
 * or username/password to authenticate the client. */

#ifndef CLIENT_USERNAME
    /*
     *!!! Please note democonfigCLIENT_PRIVATE_KEY_PEM in used for
     *!!! convenience of demonstration only.  Production devices should
     *!!! store keys securely, such as within a secure element.
     */
    #ifndef CLIENT_CERTIFICATE_PEM
        extern const uint8_t client_cert_pem_start[] asm("_binary_client_crt_start");
        extern const uint8_t client_cert_pem_end[] asm("_binary_client_crt_end");
    #endif
    #ifndef CLIENT_PRIVATE_KEY_PEM
        extern const uint8_t client_key_pem_start[] asm("_binary_client_key_start");
        extern const uint8_t client_key_pem_end[] asm("_binary_client_key_end");
    #endif
#else
    #ifndef CLIENT_PASSWORD
        #error "Please define CLIENT_PASSWORD in demo_config.h if CLIENT_USERNAME is defined."
    #endif
#endif

static const char *TAG = "AWS_IOT";

/* Forward declarations */
static int initializeMqtt(AwsIoTContext_t* ctx);
static int connectToServerWithBackoffRetries(AwsIoTContext_t* ctx);
static int establishMqttSession(AwsIoTContext_t* ctx, bool createCleanSession, bool* pSessionPresent);
static int subscribeToTopic(AwsIoTContext_t* ctx);
static int unsubscribeFromTopic(AwsIoTContext_t* ctx);
static int disconnectMqttSession(AwsIoTContext_t* ctx);
static void eventCallback(MQTTContext_t* pMqttContext, MQTTPacketInfo_t* pPacketInfo, MQTTDeserializedInfo_t* pDeserializedInfo);
static int publishReading(AwsIoTContext_t* ctx, const temp_reading_t* reading, int queue_index);

AwsIoTContext_t* aws_iot_setup(const AwsIoTConfig_t* config, QueueHandle_t* queues, size_t num_queues) {
    if (config == NULL || queues == NULL || num_queues == 0) {
        ESP_LOGE(TAG, "Invalid AWS IoT setup parameters");
        return NULL;
    }

    AwsIoTContext_t* ctx = malloc(sizeof(AwsIoTContext_t));
    if (ctx == NULL) {
        ESP_LOGE(TAG, "Failed to allocate AWS IoT context");
        return NULL;
    }

    memset(ctx, 0, sizeof(AwsIoTContext_t));
    ctx->config = *config;
    ctx->batch_queues = queues;
    ctx->num_queues = num_queues;
    ctx->is_initialized = false;

    // Initialize MQTT library
    if (initializeMqtt(ctx) != EXIT_SUCCESS) {
        ESP_LOGE(TAG, "Failed to initialize MQTT");
        free(ctx);
        return NULL;
    }

    ctx->is_initialized = true;
    ESP_LOGI(TAG, "AWS IoT context initialized successfully");
    return ctx;
}

void aws_iot_cleanup(AwsIoTContext_t* ctx) {
    if (ctx == NULL) return;

    // First mark context as not initialized to prevent new operations
    ctx->is_initialized = false;

    // Stop batch processing task if running
    if (ctx->batch_task_handle != NULL) {
        vTaskDelete(ctx->batch_task_handle);
        ctx->batch_task_handle = NULL;
    }

    // Disconnect MQTT session
    disconnectMqttSession(ctx);

    // Clean up MQTT context
    MQTT_DeInit(&ctx->mqtt_context);

    // Free context
    free(ctx);
}

static int initializeMqtt(AwsIoTContext_t* ctx) {
    MQTTStatus_t mqttStatus;

    // Initialize MQTT context
    mqttStatus = MQTT_Init(&ctx->mqtt_context,
                          &ctx->network_context,
                          true,
                          ctx->network_buffer,
                          ctx->config.network_buffer_size);

    if (mqttStatus != MQTTSuccess) {
        ESP_LOGE(TAG, "MQTT init failed: %s", MQTT_Status_strerror(mqttStatus));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int connectToServerWithBackoffRetries(AwsIoTContext_t* ctx) {
    int returnStatus = EXIT_SUCCESS;
    BackoffAlgorithmStatus_t backoffAlgStatus = BackoffAlgorithmSuccess;
    TlsTransportStatus_t opensslStatus = TLS_TRANSPORT_SUCCESS;
    BackoffAlgorithmContext_t reconnectParams;
    ServerInfo_t serverInfo;
    NetworkCredentials_t* opensslCredentials = (NetworkCredentials_t*)malloc(sizeof(NetworkCredentials_t));
    uint16_t nextRetryBackOff;

    if (opensslCredentials == NULL) {
        ESP_LOGE(TAG, "Failed to allocate network credentials");
        return EXIT_FAILURE;
    }

    // Initialize information to connect to the MQTT broker
    serverInfo.pHostName = AWS_IOT_ENDPOINT;
    serverInfo.hostNameLength = AWS_IOT_ENDPOINT_LENGTH;
    serverInfo.port = ctx->config.mqtt_port;

    // Initialize credentials for establishing TLS session
    opensslCredentials->rootCaSize = root_cert_auth_pem_end - root_cert_auth_pem_start;
    opensslCredentials->disableSni = 0;

    #ifndef CLIENT_USERNAME
        opensslCredentials->pClientCert = (const unsigned char*)client_cert_pem_start;
        opensslCredentials->clientCertSize = client_cert_pem_end - client_cert_pem_start;
        opensslCredentials->pPrivateKey = (const unsigned char*)client_key_pem_start;
        opensslCredentials->privateKeySize = client_key_pem_end - client_key_pem_start;
    #endif

    // Configure ALPN protocols based on port
    if (ctx->config.mqtt_port == 443) {
        static const char* pcAlpnProtocols[] = { NULL, NULL };
        #ifdef CLIENT_USERNAME
            pcAlpnProtocols[0] = AWS_IOT_PASSWORD_ALPN;
        #else
            pcAlpnProtocols[0] = AWS_IOT_MQTT_ALPN;
        #endif
        opensslCredentials->pAlpnProtos = pcAlpnProtocols;
    } else {
        opensslCredentials->pAlpnProtos = NULL;
    }

    // Initialize reconnect attempts and interval
    BackoffAlgorithm_InitializeParams(&reconnectParams,
                                    ctx->config.base_backoff_delay_ms,
                                    ctx->config.max_backoff_delay_ms,
                                    ctx->config.max_retry_attempts);

    // Attempt to connect to MQTT broker with exponential backoff
    do {
        ESP_LOGI(TAG, "Establishing TLS session to %.*s:%d",
                 AWS_IOT_ENDPOINT_LENGTH,
                 AWS_IOT_ENDPOINT,
                 ctx->config.mqtt_port);

        opensslStatus = TLS_FreeRTOS_Connect(&ctx->network_context,
                                           serverInfo.pHostName,
                                           serverInfo.port,
                                           opensslCredentials,
                                           ctx->config.transport_timeout_ms,
                                           ctx->config.transport_timeout_ms);

        if (opensslStatus != TLS_TRANSPORT_SUCCESS) {
            backoffAlgStatus = BackoffAlgorithm_GetNextBackoff(&reconnectParams,
                                                             &nextRetryBackOff);

            if (backoffAlgStatus == BackoffAlgorithmRetriesExhausted) {
                ESP_LOGE(TAG, "Connection to broker failed, all attempts exhausted");
                returnStatus = EXIT_FAILURE;
            } else if (backoffAlgStatus == BackoffAlgorithmSuccess) {
                ESP_LOGW(TAG, "Connection failed, retrying after %hu ms backoff",
                         (unsigned short)nextRetryBackOff);
                Clock_SleepMs(nextRetryBackOff);
            }
        }
    } while ((opensslStatus != TLS_TRANSPORT_SUCCESS) && 
             (backoffAlgStatus == BackoffAlgorithmSuccess));

    free(opensslCredentials);
    return returnStatus;
}

static int establishMqttSession(AwsIoTContext_t* ctx, bool createCleanSession, bool* pSessionPresent) {
    MQTTStatus_t mqttStatus;
    MQTTConnectInfo_t connectInfo = { 0 };

    // Set up connection info
    connectInfo.cleanSession = createCleanSession;
    connectInfo.pClientIdentifier = CLIENT_IDENTIFIER;
    connectInfo.clientIdentifierLength = CLIENT_IDENTIFIER_LENGTH;
    connectInfo.keepAliveSeconds = ctx->config.keep_alive_seconds;

    #ifdef CLIENT_USERNAME
        connectInfo.pUserName = CLIENT_USERNAME_WITH_METRICS;
        connectInfo.userNameLength = strlen(CLIENT_USERNAME_WITH_METRICS);
        connectInfo.pPassword = CLIENT_PASSWORD;
        connectInfo.passwordLength = strlen(CLIENT_PASSWORD);
    #else
        connectInfo.pUserName = METRICS_STRING;
        connectInfo.userNameLength = METRICS_STRING_LENGTH;
        connectInfo.pPassword = NULL;
        connectInfo.passwordLength = 0U;
    #endif

    // Send MQTT CONNECT packet
    mqttStatus = MQTT_Connect(&ctx->mqtt_context,
                            &connectInfo,
                            NULL,
                            ctx->config.connack_recv_timeout_ms,
                            pSessionPresent);

    if (mqttStatus != MQTTSuccess) {
        ESP_LOGE(TAG, "MQTT connection failed: %s", MQTT_Status_strerror(mqttStatus));
        return EXIT_FAILURE;
    }

    ESP_LOGI(TAG, "MQTT connection established successfully");
    return EXIT_SUCCESS;
}

static int subscribeToTopic(AwsIoTContext_t* ctx) {
    MQTTStatus_t mqttStatus;
    MQTTSubscribeInfo_t subscriptionList[2];
    size_t numTopics = 0;

    // Set up subscriptions for thermocouple topics
    subscriptionList[numTopics].qos = MQTTQoS1;
    subscriptionList[numTopics].pTopicFilter = MQTT_THERMOCOUPLE_21_TOPIC;
    subscriptionList[numTopics].topicFilterLength = MQTT_THERMOCOUPLE_21_TOPIC_LENGTH;
    numTopics++;

    subscriptionList[numTopics].qos = MQTTQoS1;
    subscriptionList[numTopics].pTopicFilter = MQTT_THERMOCOUPLE_22_TOPIC;
    subscriptionList[numTopics].topicFilterLength = MQTT_THERMOCOUPLE_22_TOPIC_LENGTH;
    numTopics++;

    // Send SUBSCRIBE packet
    mqttStatus = MQTT_Subscribe(&ctx->mqtt_context,
                               subscriptionList,
                               numTopics,
                               MQTT_GetPacketId(&ctx->mqtt_context));

    if (mqttStatus != MQTTSuccess) {
        ESP_LOGE(TAG, "Failed to subscribe to topics: %s", MQTT_Status_strerror(mqttStatus));
        return EXIT_FAILURE;
    }

    ESP_LOGI(TAG, "Subscribed to %zu topics", numTopics);
    return EXIT_SUCCESS;
}

static int unsubscribeFromTopic(AwsIoTContext_t* ctx) {
    MQTTStatus_t mqttStatus;
    MQTTSubscribeInfo_t subscriptionList[2];
    size_t numTopics = 0;

    // Set up unsubscriptions for thermocouple topics
    subscriptionList[numTopics].qos = MQTTQoS1;
    subscriptionList[numTopics].pTopicFilter = MQTT_THERMOCOUPLE_21_TOPIC;
    subscriptionList[numTopics].topicFilterLength = MQTT_THERMOCOUPLE_21_TOPIC_LENGTH;
    numTopics++;

    subscriptionList[numTopics].qos = MQTTQoS1;
    subscriptionList[numTopics].pTopicFilter = MQTT_THERMOCOUPLE_22_TOPIC;
    subscriptionList[numTopics].topicFilterLength = MQTT_THERMOCOUPLE_22_TOPIC_LENGTH;
    numTopics++;

    // Send UNSUBSCRIBE packet
    mqttStatus = MQTT_Unsubscribe(&ctx->mqtt_context,
                                 subscriptionList,
                                 numTopics,
                                 MQTT_GetPacketId(&ctx->mqtt_context));

    if (mqttStatus != MQTTSuccess) {
        ESP_LOGE(TAG, "Failed to unsubscribe from topics: %s", MQTT_Status_strerror(mqttStatus));
        return EXIT_FAILURE;
    }

    ESP_LOGI(TAG, "Unsubscribed from %zu topics", numTopics);
    return EXIT_SUCCESS;
}

static int disconnectMqttSession(AwsIoTContext_t* ctx) {
    MQTTStatus_t mqttStatus;

    mqttStatus = MQTT_Disconnect(&ctx->mqtt_context);
    if (mqttStatus != MQTTSuccess) {
        ESP_LOGE(TAG, "MQTT disconnect failed: %s", MQTT_Status_strerror(mqttStatus));
        return EXIT_FAILURE;
    }

    ESP_LOGI(TAG, "MQTT session disconnected successfully");
    return EXIT_SUCCESS;
}

static void eventCallback(MQTTContext_t* pMqttContext, MQTTPacketInfo_t* pPacketInfo, MQTTDeserializedInfo_t* pDeserializedInfo) {
    uint16_t packetIdentifier = pPacketInfo->packetIdentifier;

    if (pPacketInfo->type == MQTT_PACKET_TYPE_SUBACK) {
        ESP_LOGI(TAG, "Received SUBACK for packet ID %u", packetIdentifier);
    } else if (pPacketInfo->type == MQTT_PACKET_TYPE_UNSUBACK) {
        ESP_LOGI(TAG, "Received UNSUBACK for packet ID %u", packetIdentifier);
    } else if (pPacketInfo->type == MQTT_PACKET_TYPE_PUBLISH) {
        ESP_LOGI(TAG, "Received PUBLISH for packet ID %u", packetIdentifier);
    } else if (pPacketInfo->type == MQTT_PACKET_TYPE_PUBACK) {
        ESP_LOGI(TAG, "Received PUBACK for packet ID %u", packetIdentifier);
    } else {
        ESP_LOGW(TAG, "Received unknown packet type: 0x%02x", pPacketInfo->type);
    }
}

int publishReading(AwsIoTContext_t* ctx, const temp_reading_t* reading, int queue_index) {
    if (ctx == NULL || !ctx->is_initialized || reading == NULL) {
        ESP_LOGE(TAG, "Invalid context or reading");
        return EXIT_FAILURE;
    }

    char payload[256];
    int payloadLength = snprintf(payload, sizeof(payload),
                               "{\"sensor\":\"gpio_%d\",\"temperature\":%.2f,\"timestamp\":%ld,\"error\":%d,\"error_msg\":\"%s\"}",
                               queue_index == 0 ? 21 : 22,
                               reading->temperature_celsius,
                               (long)reading->timestamp_seconds,
                               reading->error,
                               reading->error_message);

    if (payloadLength >= sizeof(payload)) {
        ESP_LOGE(TAG, "Payload too long");
        return EXIT_FAILURE;
    }

    MQTTPublishInfo_t publishInfo = { 0 };
    publishInfo.qos = MQTTQoS1;
    publishInfo.pTopicName = queue_index == 0 ? MQTT_THERMOCOUPLE_21_TOPIC : MQTT_THERMOCOUPLE_22_TOPIC;
    publishInfo.topicNameLength = queue_index == 0 ? MQTT_THERMOCOUPLE_21_TOPIC_LENGTH : MQTT_THERMOCOUPLE_22_TOPIC_LENGTH;
    publishInfo.pPayload = payload;
    publishInfo.payloadLength = payloadLength;

    MQTTStatus_t mqttStatus = MQTT_Publish(&ctx->mqtt_context,
                                         &publishInfo,
                                         MQTT_GetPacketId(&ctx->mqtt_context));

    if (mqttStatus != MQTTSuccess) {
        ESP_LOGE(TAG, "Failed to publish reading: %s", MQTT_Status_strerror(mqttStatus));
        return EXIT_FAILURE;
    }

    ESP_LOGI(TAG, "Published reading to topic %s", publishInfo.pTopicName);
    return EXIT_SUCCESS;
}

int aws_iot_try_reconnect(AwsIoTContext_t* ctx) {
    if (ctx == NULL || !ctx->is_initialized) {
        ESP_LOGE(TAG, "Invalid or uninitialized context");
        return EXIT_FAILURE;
    }

    // Try to connect with backoff retries
    if (connectToServerWithBackoffRetries(ctx) != EXIT_SUCCESS) {
        ESP_LOGE(TAG, "Failed to connect to MQTT broker");
        return EXIT_FAILURE;
    }

    // Establish MQTT session
    bool sessionPresent = false;
    if (establishMqttSession(ctx, false, &sessionPresent) != EXIT_SUCCESS) {
        ESP_LOGE(TAG, "Failed to establish MQTT session");
        return EXIT_FAILURE;
    }

    // Subscribe to topic
    if (subscribeToTopic(ctx) != EXIT_SUCCESS) {
        ESP_LOGE(TAG, "Failed to subscribe to topic");
        // Don't return failure here, as we're already connected
        // Just log the error and continue
    }

    return EXIT_SUCCESS;
}

void aws_iot_task(void* param) {
    AwsIoTContext_t* ctx = (AwsIoTContext_t*)param;
    if (ctx == NULL || !ctx->is_initialized) {
        ESP_LOGE(TAG, "Invalid context");
        vTaskDelete(NULL);
        return;
    }

    // Connect to AWS IoT
    if (aws_iot_try_reconnect(ctx) != EXIT_SUCCESS) {
        ESP_LOGE(TAG, "Failed to connect to AWS IoT");
        vTaskDelete(NULL);
        return;
    }

    // Main task loop
    while (ctx->is_initialized) {
        // Process MQTT packets
        MQTTStatus_t mqttStatus = MQTT_ProcessLoop(&ctx->mqtt_context,
                                                 ctx->config.process_loop_timeout_ms);

        if (mqttStatus != MQTTSuccess) {
            ESP_LOGW(TAG, "MQTT process loop failed: %s", MQTT_Status_strerror(mqttStatus));
            
            // Try to reconnect
            if (aws_iot_try_reconnect(ctx) != EXIT_SUCCESS) {
                ESP_LOGE(TAG, "Failed to reconnect to AWS IoT");
                vTaskDelay(pdMS_TO_TICKS(5000));  // Wait before retrying
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // Small delay to prevent tight loop
    }

    vTaskDelete(NULL);
} 