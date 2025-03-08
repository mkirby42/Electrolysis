#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "fixed_point.h"
#include "fir_filter.h"

// Cycle counter for ARM Cortex-M4
static inline uint32_t get_cycles(void) {
    uint32_t cycles;
    asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (cycles));
    return cycles;
}

typedef struct {
    const char* name;
    uint32_t cycles;
    double time_ms;
    size_t input_size;
} benchmark_result_t;

#define MAX_RESULTS 100
static benchmark_result_t results[MAX_RESULTS];
static size_t num_results = 0;

void add_result(const char* name, uint32_t cycles, double time_ms, size_t input_size) {
    if (num_results < MAX_RESULTS) {
        results[num_results].name = name;
        results[num_results].cycles = cycles;
        results[num_results].time_ms = time_ms;
        results[num_results].input_size = input_size;
        num_results++;
    }
}

void benchmark_fir_filter(const float* input, size_t input_size, const float* coeffs, size_t num_coeffs) {
    float* output = malloc(input_size * sizeof(float));
    
    uint32_t start_cycles = get_cycles();
    clock_t start = clock();
    
    fir_filter_f32(input, output, input_size, coeffs, num_coeffs);
    
    clock_t end = clock();
    uint32_t end_cycles = get_cycles();
    
    double time_ms = ((double)(end - start) * 1000.0) / CLOCKS_PER_SEC;
    add_result("FIR Filter (float)", end_cycles - start_cycles, time_ms, input_size);
    
    free(output);
}

void benchmark_fixed_point_fir(const int32_t* input, size_t input_size, const int32_t* coeffs, size_t num_coeffs) {
    int32_t* output = malloc(input_size * sizeof(int32_t));
    
    uint32_t start_cycles = get_cycles();
    clock_t start = clock();
    
    fir_filter_fixed(input, output, input_size, coeffs, num_coeffs);
    
    clock_t end = clock();
    uint32_t end_cycles = get_cycles();
    
    double time_ms = ((double)(end - start) * 1000.0) / CLOCKS_PER_SEC;
    add_result("FIR Filter (fixed)", end_cycles - start_cycles, time_ms, input_size);
    
    free(output);
}

void print_results(void) {
    printf("\nBenchmark Results:\n");
    printf("%-20s %-15s %-15s %-15s %-15s\n", 
           "Test", "Cycles", "Time (ms)", "Input Size", "Cycles/Sample");
    printf("----------------------------------------------------------------\n");
    
    for (size_t i = 0; i < num_results; i++) {
        printf("%-20s %-15u %-15.3f %-15zu %-15.2f\n",
               results[i].name,
               results[i].cycles,
               results[i].time_ms,
               results[i].input_size,
               (double)results[i].cycles / results[i].input_size);
    }
}

int main(void) {
    // Test data
    const size_t input_sizes[] = {128, 256, 512, 1024, 2048};
    const size_t num_sizes = sizeof(input_sizes) / sizeof(input_sizes[0]);
    
    // FIR filter coefficients (simple low-pass)
    const float coeffs_f32[] = {0.1f, 0.2f, 0.4f, 0.2f, 0.1f};
    const int32_t coeffs_fixed[] = {3277, 6554, 13107, 6554, 3277}; // Q15 format
    const size_t num_coeffs = 5;
    
    for (size_t i = 0; i < num_sizes; i++) {
        const size_t size = input_sizes[i];
        
        // Generate test data
        float* input_f32 = malloc(size * sizeof(float));
        int32_t* input_fixed = malloc(size * sizeof(int32_t));
        
        for (size_t j = 0; j < size; j++) {
            input_f32[j] = (float)rand() / RAND_MAX;
            input_fixed[j] = (int32_t)(input_f32[j] * 32768.0f); // Convert to Q15
        }
        
        // Run benchmarks
        benchmark_fir_filter(input_f32, size, coeffs_f32, num_coeffs);
        benchmark_fixed_point_fir(input_fixed, size, coeffs_fixed, num_coeffs);
        
        free(input_f32);
        free(input_fixed);
    }
    
    print_results();
    return 0;
} 