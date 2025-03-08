#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "fir_filter.h"

#define MAX_SAMPLES 10000

int main() {
    float input[MAX_SAMPLES];
    float output[MAX_SAMPLES];
    size_t num_samples = 0;
    
    // Read input data
    FILE *fp = fopen("../data/input_signal.txt", "r");
    if (!fp) {
        fprintf(stderr, "Failed to open input file\n");
        return 1;
    }
    
    while (num_samples < MAX_SAMPLES && fscanf(fp, "%f", &input[num_samples]) == 1) {
        num_samples++;
    }
    fclose(fp);
    
    // Initialize filter (example parameters - adjust based on your needs)
    const float coeffs[] = {0.1f, 0.2f, 0.4f, 0.2f, 0.1f};
    const size_t num_coeffs = sizeof(coeffs) / sizeof(coeffs[0]);
    
    // Benchmark
    clock_t start = clock();
    
    fir_filter(input, num_samples, coeffs, num_coeffs, output);
    
    clock_t end = clock();
    double cpu_time = ((double) (end - start)) / CLOCKS_PER_SEC;
    
    // Save output
    fp = fopen("../data/c_output.txt", "w");
    if (!fp) {
        fprintf(stderr, "Failed to open output file\n");
        return 1;
    }
    
    for (size_t i = 0; i < num_samples; i++) {
        fprintf(fp, "%f\n", output[i]);
    }
    fclose(fp);
    
    printf("Processing time: %f seconds\n", cpu_time);
    printf("Samples processed: %zu\n", num_samples);
    printf("Samples per second: %f\n", num_samples / cpu_time);
    
    return 0;
} 