## Phase 1
Goal: Implement FreeRTOS on ESP32 to manage tasks related to electrolysis reactor data collection while strengthening C, real time computing and networking skills

### Week 1
Objective: Get familiar with FreeRTOS on ESP32

Read up on FreeRTOS 
- Hand On RTOS w micro controllers

Write a basic FreeRTOS program on ESP32
Create two tasks 
- read the sensor
- log the data
- use `vTaskDelay()` to control task execution and timing

Deliverable: Working ESP32 FreeRTOS w two tasks

### Week 2
Objective: learn task synchronization, mutexes, and interrupt handling

- Use mutexes or semaphores to avoid conflicts between tasks accessing shared data
- Implement an ISR to trigger a task when new data arrives
- Read more of RTOS book
- Read about priority inversion and real time scheduling 

Deliverable: A real time data acquisition system with FreeRTOS tasks and ISR

### Week 3
Objective: Optimize memory usage, reduce latency, and stream sensor data efficiently

- Optimize heap and stack allocation
- Implement ring buffer for sensor data storage
- Compression
- The art of writing efficient programs

Deliverable: Low latency data pipeline for sensor readings with optimized memory allocation

### Week 4
Objective: Set up real time data streaming from ESP32 to a server & dashboard

- UDP data stream from ESP to computer
- ZeroMQ or MQTT for low latency telemetry 
- wifi v direct serial
- Experiment with Time sensitive networking features on ESP32
- Read about real time message passing (pub sub)

Deliverable: ESP32 streams real time sensor data to server

### Week 5
Objective: Implement real time control of electrolysis parameters based on sensor feedback

- Add a feedback loop where sensor readings adjust power to the reactor
- Implement PID control in C to stabilize temperature and gas production rates
- Study real time scheduling algorithms (rate monitoring, earliest deadline first)

Deliverable: A working feedback loop where ESP dynamically adjusts reactor operation

### Week 6
Objective: Optimize real time performance 

- Profile ESP memory/CPU usage
- Optimize ISR execution time
- Implement watchdog timer & fault recovery
- Read about zero copy data transfer techniques

Deliverable: Optimized real time software for stable electrolysis control