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

- [x] Hand On RTOS w micro controllers Chapter 1: Introducing Real Time Systems
- [ ] Hand On RTOS w micro controllers Chapter 2: Understanding RTOS Tasks
- [x] The Art of Writing Efficient Programs Chapter 1: Introduction to Performance and Concurrency
- [x] [Map Reduce: Simplified Data Processing on Large Clusters](https://pdos.csail.mit.edu/6.824/papers/mapreduce.pdf)
- [ ] [The Google File System](https://pdos.csail.mit.edu/6.824/papers/gfs.pdf)
- [x] Notes on Redis
- [x] Deliverable: Working ESP32 FreeRTOS w two tasks
- [x] Deliverable: Working ESP32 FreeRTOS w AWS IoT Integration
- [x] Deliverable: Working electrolysis reactor

### Week 2
Objective: learn task synchronization, mutexes, and interrupt handling

- Use mutexes or semaphores to avoid conflicts between tasks accessing shared data
- Implement an ISR to trigger a task when new data arrives
- Read more of RTOS book
- Read about priority inversion and real time scheduling 

- [ ] Notes on DynamoDB
- [ ] The Art of Writing Efficient Programs Chapter 2: Performance Measurements
- [ ] The Art of Writing Efficient Programs Chapter 3: CPU Architectures, Resources, and Performance Implications
- [ ] Deliverable: A real time data acquisition system with FreeRTOS tasks and ISR
- [ ] [The Design of a Practical System for Fault-Tolerant Virtual Machines](https://pdos.csail.mit.edu/6.824/papers/vm-ft.pdf)
- [ ] [In search of an Understandable Consensus Algorithm](https://pdos.csail.mit.edu/6.824/papers/raft-extended.pdf)

### Week 3
Objective: Optimize memory usage, reduce latency, and stream sensor data efficiently

- Optimize heap and stack allocation
- Implement ring buffer for sensor data storage
- Compression
- The art of writing efficient programs

- [ ] The Art of Writing Efficient Programs Chapter 4: Memory Architecture and Performance
- [ ] The Art of Writing Efficient Programs Chapter 5: Threads, Memory, and Concurrency 
- [ ] Deliverable: Low latency data pipeline for sensor readings with optimized memory allocation
- [ ] [ZooKeeper: Wait-free coordination for Internet-scale systems](https://pdos.csail.mit.edu/6.824/papers/zookeeper.pdf)
 - [ ] [Chapter 9: Atomicity: All-or-nothing and Before-or-after](https://ocw.mit.edu/courses/res-6-004-principles-of-computer-system-design-an-introduction-spring-2009/resources/atomicity_open_5_0/)

### Week 4
Objective: Set up real time data streaming from ESP32 to a server & dashboard

- UDP data stream from ESP to computer
- ZeroMQ or MQTT for low latency telemetry 
- wifi v direct serial
- Experiment with Time sensitive networking features on ESP32
- Read about real time message passing (pub sub)

- [ ] [AWS MQTT](https://docs.aws.amazon.com/iot/latest/developerguide/mqtt.html)
= [ ] [Spanner: Google’s Globally-Distributed Database](https://pdos.csail.mit.edu/6.824/papers/spanner.pdf)
- [ ] [No compromises: distributed transactions with consistency, availability, and performance](https://pdos.csail.mit.edu/6.824/papers/farm-2015.pdf)
- [ ] Deliverable: ESP32 streams real time sensor data to server

### Week 5
Objective: Implement real time control of electrolysis parameters based on sensor feedback

- Add a feedback loop where sensor readings adjust power to the reactor
- Implement PID control in C to stabilize temperature and gas production rates
- Study real time scheduling algorithms (rate monitoring, earliest deadline first)

- [ ] [Chardonnay: Fast and General Datacenter Transactions for On-Disk Databases](https://pdos.csail.mit.edu/6.824/papers/osdi23-eldeeb.pdf)
 - [ ] [Amazon DynamoDB: A Scalable, Predictably Performant, and Fully Managed NoSQL Database Service](https://pdos.csail.mit.edu/6.824/papers/atc22-dynamodb.pdf)
- [ ] Deliverable: A working feedback loop where ESP dynamically adjusts reactor operation

### Week 6
Objective: Optimize real time performance 

- Profile ESP memory/CPU usage
- Optimize ISR execution time
- Implement watchdog timer & fault recovery
- Read about zero copy data transfer techniques

- [ ] [Ownership: A Distributed Futures System for Fine-Grained Tasks](https://pdos.csail.mit.edu/6.824/papers/ray.pdf)
- [ ] [Scaling Memcache at Facebook](https://pdos.csail.mit.edu/6.824/papers/memcache-fb.pdf) 
- [ ] Deliverable: Optimized real time software for stable electrolysis control

### More Papers
= [ ] [Grove: a Separation-Logic Library for Verifying Distributed Systems](https://pdos.csail.mit.edu/6.824/papers/grove.pdf)
- [ ] [On-demand Container Loading in AWS Lambda](https://pdos.csail.mit.edu/6.824/papers/atc23-brooker.pdf)- [ ] [Boki: Stateful Serverless Computing with Shared Logs](https://pdos.csail.mit.edu/6.824/papers/jia21sosp-boki.pdf)
- [ ] [Secure Untrusted Data Repository (SUNDR)](https://pdos.csail.mit.edu/6.824/papers/li-sundr.pdf)
- [ ] [Practical Byzantine Fault Tolerance](https://pdos.csail.mit.edu/6.824/papers/castro-practicalbft.pdf)
- [ ] [Bitcoin: A Peer-to-Peer Electronic Cash System](https://pdos.csail.mit.edu/6.824/papers/bitcoin.pdf)