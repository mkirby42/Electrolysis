#ifndef FIR_FILTER_H
#define FIR_FILTER_H

#include "fixed_point.h"
#include <stddef.h>

// Error codes
typedef enum {
    SP_SUCCESS = 0,
    SP_ERROR_NULL_POINTER = -1,
    SP_ERROR_MEMORY_ALLOCATION = -2,
    SP_ERROR_BUFFER_TOO_SMALL = -3
} sp_error_t;

// Signal structure
typedef struct {
    fixed_point_t* data;
    size_t length;
} sp_signal_t;

// FIR filter structure
typedef struct {
    fixed_point_t* coefficients;
    fixed_point_t* buffer;
    size_t length;
    size_t current_index;
} sp_fir_filter_t;

// Benchmarking structure
typedef struct {
    const char* function_name;
    uint32_t start_cycles;
    uint32_t end_cycles;
    uint32_t total_cycles;
} sp_benchmark_t;

// FIR filter functions
sp_error_t sp_fir_init(sp_fir_filter_t* filter, const fixed_point_t* coefficients, size_t length);
sp_error_t sp_fir_process(sp_fir_filter_t* filter, const sp_signal_t* input, sp_signal_t* output);
void sp_fir_destroy(sp_fir_filter_t* filter);

// Benchmarking functions
void sp_benchmark_start(sp_benchmark_t* bench, const char* function_name);
void sp_benchmark_end(sp_benchmark_t* bench);
void sp_benchmark_print(const sp_benchmark_t* bench);

#endif // FIR_FILTER_H 