import json
import time
from awscrt import mqtt
from awsiot import mqtt_connection_builder

# AWS IoT configuration
ENDPOINT = "a1hlr2thig51kw-ats.iot.us-east-1.amazonaws.com"
CLIENT_ID = "python-subscriber"
TOPIC_21 = "thermocouple/21/readings"
TOPIC_22 = "thermocouple/22/readings"
CERT_PATH = "main/certs/client.crt"
KEY_PATH = "main/certs/client.key"
ROOT_CA = "main/certs/root_cert_auth.pem"

def on_message(topic, payload, **kwargs):
    try:
        data = json.loads(payload.decode())
        print(f"\nReceived data on topic {topic}:")
        print(json.dumps(data, indent=2))
    except json.JSONDecodeError:
        print(f"Failed to decode JSON from topic {topic}")
    except Exception as e:
        print(f"Error processing message from topic {topic}: {e}")

# Create MQTT connection
mqtt_connection = mqtt_connection_builder.mtls_from_path(
    endpoint=ENDPOINT,
    cert_filepath=CERT_PATH,
    pri_key_filepath=KEY_PATH,
    ca_filepath=ROOT_CA,
    client_id=CLIENT_ID,
    clean_session=False,
    keep_alive_secs=30
)

print("Connecting to AWS IoT Core...")
connect_future = mqtt_connection.connect()
connect_future.result()
print("Connected!")

# Subscribe to both thermocouple topics
for topic in [TOPIC_21, TOPIC_22]:
    subscribe_future, _ = mqtt_connection.subscribe(
        topic=topic,
        qos=mqtt.QoS.AT_LEAST_ONCE,
        callback=on_message
    )
    subscribe_future.result()  # Wait for subscription to complete
    print(f"Subscribed to {topic}")

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("Disconnecting...")
    disconnect_future = mqtt_connection.disconnect()
    disconnect_future.result()