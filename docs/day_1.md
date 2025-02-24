# Day 1 
Sunday 2-21-2025

Get dev environment working 

Let's set up the ESP IDF 

Clone the repo
```bash
git clone --recursive https://github.com/espressif/esp-idf.git
```
Change directory
```bash
cd esp-idf
```

# Run install script
```bash
./install.sh
```

## Step 2: Setup the ESP-IDF environment
```bash
source $HOME/esp/esp-idf/export.sh
```
I got this and I want to use a different python version
```bash
Checking "python3" ...
Python 3.13.1
"python3" has been detected
Activating ESP-IDF 5.5
Setting IDF_PATH to '/Users/matt/esp-idf'.
* Checking python version ... 3.13.1
* Checking python dependencies ... OK
* Deactivating the current ESP-IDF environment (if any) ... OK
* Establishing a new ESP-IDF environment ... OK
* Identifying shell ... zsh
* Detecting outdated tools in system ... Found tools that are not used by active ESP-IDF version.
For removing old versions of xtensa-esp-elf, openocd-esp32, esp-rom-elfs, xtensa-esp-elf-gdb use command 'python /Users/matt/esp-idf/tools/idf_tools.py uninstall'
To free up even more space, remove installation packages of those tools.
Use option python /Users/matt/esp-idf/tools/idf_tools.py uninstall --remove-archives.
* Shell completion ... Autocompletion code generated

Done! You can now compile ESP-IDF projects.
Go to the project directory and run:

  idf.py build
```
so i should be able to run 
```bash
export IDF_PYTHON_ENV_PATH="/opt/homebrew/bin/python3.12"
source ~/esp-idf/export.sh
```
but it concatenates the paths sso let's try to clean that up
```bash
unset IDF_PYTHON_ENV_PATH
unset IDF_PATH
export IDF_PYTHON_ENV_PATH="/opt/homebrew/bin/python3.12"
source ~/esp-idf/export.sh
```
that didn';t work
```bash
rm -rf ~/.espressif/python_env

```
I'm going to try and edit the export.sh directly as nothing seems to be working to get it to use the right python version.
Looks like I have to modify 
```bash
. "${idf_path}/tools/detect_python.sh"`
```
oh seems like 3.13 is supported, maybe I should just roll with it?

`detect_python.sh`
```bash
for p_cmd in python3 python python3.9 python3.10 python3.11 python3.12 python3.13; do
```

yeesh ok 
```bash
unset IDF_PYTHON_ENV_PATH
unset IDF_PATH
rm -rf ~/.espressif/python_env    
./install.sh     
source ~/esp-idf/export.sh
```

Add the IDF path to your bash profile so you don't have to run the export script every time you open a new terminal
```bash
echo 'export PATH="$HOME/esp-idf/tools:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

## Step 3: Install the toolchain 
CMake Ninja and Python dependencies 
```bash
python -m pip install -r $IDF_PATH/tools/requirements/requirements.core.txt
```

### Step 4: Verify the installation
```bash
idf.py --version
```
```bash
ESP-IDF v5.5-dev-2038-g81e8b752fda
```

compile a test program
```bash
cd $IDF_PATH/examples/get-started/blink
idf.py set-target esp32
idf.py build
```

flash to the ESP32
replace /dev/ttyUSB0 with the port your ESP32 is connected to
```bash
idf.py -p /dev/cu.usbserial-02054FDD flash monitor
```

monitor the serial output
```bash
idf.py monitor
```
[Video](static/video/esp32_blink.mp4)
Sweet!

Let's see if we can make a new project for our RTOS proof of concept.

Let's step through some of the program components
First we have a temperature buffer `temperatureBuffer` set to store `BUFFER_SIZE` float samples `float temperatureBuffer[BUFFER_SIZE];`

Next is a FreeRTOS Queue for inter-task comms
`QueueHandle_t tempQueue;`

After those we have the first function `readInternalTemperature`
This uses a one shot adc sample to read the internal temperature sensor # This esp doesn't have one of those
We setup the adc1 for 12 bit sampling `adc1_config_width(ADC_WIDTH_BIT_12);`
Use channel 0 `adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);`
Then get the sample `raw_adc = adc1_get_raw(ADC1_CHANNEL_0);`
After this we convert the 12 bit adc reading to a floating point temperature value `float temperature = (raw_adc / 4096.0) * 100.0;`
Finally we return the value

The next function defines our first RTOS task `readTemperatureTask`
It consists of a forever while loop `while (1) {...}`
Inside the loop we call the `readInternalTemperature` function 
We log the returned temperature value `ESP_LOGI(TAG, "Temperature Read: %.2f°C", temperature);`
Then we send the value to the queue
`xQueueSend(tempQueue, &temperature, portMAX_DELAY);`
We then use `vTaskDelay` to delay the next execution of the task for `TEMP_READ_INTERVAL_MS` which we set as a global variable 
`vTaskDelay(pdMS_TO_TICKS(TEMP_READ_INTERVAL_MS));`

The second task similarly starts with a forever while loop
In this we check if the queue has received a value with `xQueueReceive` `if (xQueueReceive(tempQueue, &receivedTemp, portMAX_DELAY)) {...}`
Inside the check we append the received value to the buffer `temperatureBuffer[bufferIndex] = receivedTemp;`
Then we ensure we circle back to the start of our buffer `bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;` and log the action

Finally inside the main loop we log the start of the program, and initialize a queue withxQueueCreate to allow for the tasks to pass data to eachother `tempQueue = xQueueCreate(5, sizeof(float));`

We then initialize our two tasks with xTaskCreate with task 1 'ReadTempTask' having a higher priority (2) than 'StoreTempTask' (1)
`xTaskCreate(readTemperatureTask, "ReadTempTask", 2048, NULL, 2, NULL);` 
`xTaskCreate(storeTemperatureTask, "StoreTempTask", 2048, NULL, 1, NULL);`











