# ESP32 DS18B20 Temperature Sensor with AWS IoT Integration

This project combines DS18B20 temperature sensor functionality with AWS IoT connectivity on an ESP32 microcontroller. It reads temperature data from a DS18B20 digital temperature sensor and publishes the readings to AWS IoT Core via MQTT.

## Features

- DS18B20 temperature sensor reading using the 1-Wire protocol
- Temperature storage in a local circular buffer
- AWS IoT Core integration using MQTT with TLS mutual authentication
- Real-time temperature data publishing to AWS IoT
- Configurable temperature reading interval
- Robust error handling and reconnection logic

## Hardware Requirements

- ESP32 development board (ESP32-DevKitC, NodeMCU-ESP32, etc.)
- DS18B20 temperature sensor
- 4.7kΩ pull-up resistor
- Breadboard and jumper wires
- USB cable for programming and power

## Wiring

Connect the DS18B20 temperature sensor to the ESP32 as follows:
- VCC: 3.3V
- GND: GND
- DATA: GPIO 4 (with 4.7kΩ pull-up resistor to VCC)

## Software Setup

### Prerequisites

1. ESP-IDF v4.4 or later
2. AWS IoT Core account and configured Thing
3. Generated certificates for AWS IoT authentication

### AWS IoT Setup

1. Create a Thing in AWS IoT Core
2. Create and download the certificates
3. Create a policy that allows publishing and subscribing to topics
4. Attach the policy to your certificate
5. Place the certificates in the `main/certs` directory:
   - `root_cert_auth.pem`: AWS IoT Root CA
   - `client.crt`: Device certificate
   - `client.key`: Device private key

### Project Configuration

Configure the project using `idf.py menuconfig`:

1. Configure Wi-Fi credentials under "Example Connection Configuration"
2. Configure AWS IoT endpoint under "AWS IoT MQTT Configuration"
3. Adjust other parameters as needed

## Building and Flashing

```bash
# Clone the repository
git clone <repository-url>
cd rtos_ds18b20_aws

# Configure
idf.py menuconfig

# Build
idf.py build

# Flash
idf.py -p <port> flash

# Monitor
idf.py -p <port> monitor
```

## Usage

After flashing the firmware, the ESP32 will:
1. Connect to the configured Wi-Fi network
2. Initialize the DS18B20 temperature sensor
3. Establish a secure connection to AWS IoT Core
4. Start reading temperature at the configured interval
5. Publish temperature readings to the `device/temperature` topic in JSON format:
   ```json
   {
     "temperature_c": 23.5,
     "temperature_f": 74.3,
     "timestamp": 1615123456
   }
   ```

## Project Structure

- `main/app_main.c`: Main application entry point, initializes components
- `main/rtos_ds18b20_sensor.c`: DS18B20 sensor interface and reading functions
- `main/mqtt_demo_mutual_auth.c`: AWS IoT connectivity and MQTT functionality
- `main/certs/`: Certificates for AWS IoT authentication
- Configuration files for project settings

## Troubleshooting

- If the sensor is not detected, check the wiring and pull-up resistor
- For connectivity issues, verify Wi-Fi credentials and AWS IoT endpoint
- Check the monitor output for detailed error messages
- Verify that certificates are correctly placed in the `certs` directory
- Ensure the policy attached to your certificate has sufficient permissions

## License

[Insert your chosen license here] 


source ~/esp-idf/export.sh & cd rtos_ds18b20_aws & idf.py monitor