#include "../src/fir_filter.h"
#include "../src/fixed_point.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

// Simple moving average test
void test_moving_average() {
    printf("Starting test\n");
    const size_t filter_length = 4;
    
    // Create coefficients on heap
    fixed_point_t* coeffs = malloc(filter_length * sizeof(fixed_point_t));
    if (!coeffs) {
        printf("Failed to allocate coeffs\n");
        return;
    }
    printf("Allocated coeffs\n");
    
    // 4-point moving average filter
    for (size_t i = 0; i < filter_length; i++) {
        coeffs[i] = sp_float_to_fixed(0.25f);
        printf("Coeff[%zu] = %f\n", i, sp_fixed_to_float(coeffs[i]));
    }
    printf("Initialized coeffs\n");

    sp_fir_filter_t filter;
    printf("Calling init\n");
    sp_error_t err = sp_fir_init(&filter, coeffs, filter_length);
    printf("Init returned: %d\n", err);
    if (err != SP_SUCCESS) {
        printf("Filter init failed\n");
        free(coeffs);
        return;
    }
    free(coeffs); // Safe to free after init copies them
    printf("Freed coeffs\n");

    // Test signal: unit impulse
    const size_t signal_length = 8;
    fixed_point_t* input_data = malloc(signal_length * sizeof(fixed_point_t));
    fixed_point_t* output_data = malloc(signal_length * sizeof(fixed_point_t));
    if (!input_data || !output_data) {
        printf("Failed to allocate signals\n");
        sp_fir_destroy(&filter);
        free(input_data);
        free(output_data);
        return;
    }
    printf("Allocated signals\n");

    // Initialize input signal
    input_data[0] = sp_float_to_fixed(1.0f);
    for (size_t i = 1; i < signal_length; i++) {
        input_data[i] = sp_float_to_fixed(0.0f);
    }
    printf("Initialized input\n");

    sp_signal_t input = {input_data, signal_length};
    sp_signal_t output = {output_data, signal_length};

    printf("Calling process\n");
    err = sp_fir_process(&filter, &input, &output);
    printf("Process returned: %d\n", err);
    if (err != SP_SUCCESS) {
        printf("Process failed\n");
        sp_fir_destroy(&filter);
        free(input_data);
        free(output_data);
        return;
    }

    // Expected output for moving average
    float expected[8] = {0.25f, 0.25f, 0.25f, 0.25f, 0.0f, 0.0f, 0.0f, 0.0f};
    
    printf("Checking results\n");
    for (size_t i = 0; i < signal_length; i++) {
        float result = sp_fixed_to_float(output_data[i]);
        printf("Sample %zu: got %f, expected %f\n", i, result, expected[i]);
        assert(fabsf(result - expected[i]) < 0.01f);  // Allow small fixed-point error
    }

    // Cleanup
    printf("Cleaning up\n");
    sp_fir_destroy(&filter);
    free(input_data);
    free(output_data);
    printf("Moving average test passed!\n");
}

int main() {
    printf("Starting main\n");
    test_moving_average();
    printf("Test complete\n");
    return 0;
} 