#include "../include/signal_proc.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIGNAL_LENGTH 10000
#define FILTER_LENGTHS_COUNT 4
#define ITERATIONS 100

void benchmark_fir_filter(size_t filter_length, size_t signal_length) {
    // Create filter coefficients
    fixed_point_t* coeffs = malloc(filter_length * sizeof(fixed_point_t));
    for (size_t i = 0; i < filter_length; i++) {
        coeffs[i] = sp_float_to_fixed(1.0f / filter_length);
    }

    // Initialize filter
    sp_fir_filter_t filter;
    sp_fir_init(&filter, coeffs, filter_length);

    // Create input signal
    fixed_point_t* input_data = malloc(signal_length * sizeof(fixed_point_t));
    fixed_point_t* output_data = malloc(signal_length * sizeof(fixed_point_t));
    for (size_t i = 0; i < signal_length; i++) {
        input_data[i] = sp_float_to_fixed((float)rand() / RAND_MAX);
    }

    sp_signal_t input = {input_data, signal_length};
    sp_signal_t output = {output_data, signal_length};

    // Benchmark
    sp_benchmark_t bench;
    sp_benchmark_start(&bench, "fir_filter");
    
    for (int i = 0; i < ITERATIONS; i++) {
        sp_fir_process(&filter, &input, &output);
    }
    
    sp_benchmark_end(&bench);

    // Calculate and print metrics
    float cycles_per_sample = (float)bench.total_cycles / (signal_length * ITERATIONS);
    printf("Filter length: %zu\n", filter_length);
    printf("Total cycles: %u\n", bench.total_cycles);
    printf("Cycles per sample: %.2f\n", cycles_per_sample);
    printf("--------------------\n");

    // Cleanup
    sp_fir_destroy(&filter);
    free(coeffs);
    free(input_data);
    free(output_data);
}

int main() {
    srand(time(NULL));
    
    size_t filter_lengths[FILTER_LENGTHS_COUNT] = {4, 16, 64, 256};
    
    printf("Running FIR filter benchmarks...\n\n");
    
    for (int i = 0; i < FILTER_LENGTHS_COUNT; i++) {
        benchmark_fir_filter(filter_lengths[i], SIGNAL_LENGTH);
    }
    
    return 0;
} 