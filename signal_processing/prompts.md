I need to create a signal processing library in c for use in an embedded system specifically cypress psoc 6 running an ARM cortex m1 and an ARM cortex M0+

The goal here is twofold. 
1. Produce a useful signal processing library to replicate functionality that the data science team uses for respiration algorithms (fir filters, interpolations, resampling, peak detection, fft)

2. Learn more about program optimization. Benchmarking profiling etc. I'm currently reading the art of writing efficient programs and intend this library to be the testbed for the concepts i learn there

Is there a way i can 
- Generate data (parameterized signal with noise injected) with python
- Run data through python implementation (butter filter)
- Save results
- Compile c code for my target platform
- Run data through c signal processing code on an emulated ARM cortex 
- Extract benchmark and profiling data 
- Ensure outputs are correct vs the python