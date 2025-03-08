#ifndef SIGNAL_PROC_H
#define SIGNAL_PROC_H

#include <stdint.h>
#include <stddef.h>

// Configuration for fixed-point arithmetic
#define FIXED_POINT_BITS 16
typedef int32_t fixed_point_t;

// Error codes
typedef enum {
    SP_SUCCESS = 0,
    SP_ERROR_NULL_POINTER,
    SP_ERROR_INVALID_PARAMETER,
    SP_ERROR_BUFFER_TOO_SMALL,
    SP_ERROR_MEMORY_ALLOCATION
} sp_error_t;

// Core data structures
typedef struct {
    fixed_point_t* coefficients;
    size_t length;
    size_t current_index;
    fixed_point_t* buffer;
} sp_fir_filter_t;

typedef struct {
    fixed_point_t* data;
    size_t length;
} sp_signal_t;

// FIR Filter functions
sp_error_t sp_fir_init(sp_fir_filter_t* filter, const fixed_point_t* coefficients, size_t length);
sp_error_t sp_fir_process(sp_fir_filter_t* filter, const sp_signal_t* input, sp_signal_t* output);
void sp_fir_destroy(sp_fir_filter_t* filter);

// Peak detection
sp_error_t sp_find_peaks(const sp_signal_t* signal, size_t* peaks, size_t* num_peaks, 
                        size_t max_peaks, fixed_point_t threshold);

// Interpolation
sp_error_t sp_linear_interpolate(const sp_signal_t* input, sp_signal_t* output, 
                                size_t new_length);

// FFT (using fixed-point arithmetic)
sp_error_t sp_fft(const sp_signal_t* input, sp_signal_t* real_output, 
                  sp_signal_t* imag_output);

// Utility functions
fixed_point_t sp_float_to_fixed(float value);
float sp_fixed_to_float(fixed_point_t value);

// Benchmarking utilities
typedef struct {
    uint32_t start_cycles;
    uint32_t end_cycles;
    uint32_t total_cycles;
    const char* function_name;
} sp_benchmark_t;

void sp_benchmark_start(sp_benchmark_t* bench, const char* function_name);
void sp_benchmark_end(sp_benchmark_t* bench);
void sp_benchmark_print(const sp_benchmark_t* bench);

#endif // SIGNAL_PROC_H 