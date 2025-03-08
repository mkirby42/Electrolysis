#include "fir_filter.h"
#include <stdlib.h>
#include <string.h>

#ifdef __ARM_ARCH
// Cortex-M cycle counter access
#define DWT_CTRL    (*(volatile uint32_t*)0xE0001000)
#define DWT_CYCCNT  (*(volatile uint32_t*)0xE0001004)
#endif

sp_error_t sp_fir_init(sp_fir_filter_t* filter, const fixed_point_t* coefficients, size_t length) {
    if (!filter || !coefficients || length == 0) {
        return SP_ERROR_NULL_POINTER;
    }

    filter->coefficients = (fixed_point_t*)malloc(length * sizeof(fixed_point_t));
    filter->buffer = (fixed_point_t*)malloc(length * sizeof(fixed_point_t));
    
    if (!filter->coefficients || !filter->buffer) {
        free(filter->coefficients);
        free(filter->buffer);
        return SP_ERROR_MEMORY_ALLOCATION;
    }

    memcpy(filter->coefficients, coefficients, length * sizeof(fixed_point_t));
    memset(filter->buffer, 0, length * sizeof(fixed_point_t));
    
    filter->length = length;
    filter->current_index = 0;
    
    return SP_SUCCESS;
}

sp_error_t sp_fir_process(sp_fir_filter_t* filter, const sp_signal_t* input, sp_signal_t* output) {
    if (!filter || !input || !output || !input->data || !output->data) {
        return SP_ERROR_NULL_POINTER;
    }

    if (output->length < input->length) {
        return SP_ERROR_BUFFER_TOO_SMALL;
    }

    sp_benchmark_t bench;
    sp_benchmark_start(&bench, "fir_process");

    for (size_t i = 0; i < input->length; i++) {
        // Update circular buffer
        filter->buffer[filter->current_index] = input->data[i];
        
        // Calculate output sample
        int64_t acc = 0;  // Use 64-bit accumulator to prevent overflow
        size_t buf_idx = filter->current_index;
        
        // Process all coefficients
        for (size_t j = 0; j < filter->length; j++) {
            acc += (int64_t)filter->buffer[buf_idx] * filter->coefficients[j];
            buf_idx = (buf_idx == 0) ? filter->length - 1 : buf_idx - 1;
        }
        
        // Scale result back to fixed-point
        output->data[i] = (fixed_point_t)(acc >> FIXED_POINT_BITS);
        
        // Update circular buffer index
        filter->current_index = (filter->current_index + 1) % filter->length;
    }

    sp_benchmark_end(&bench);
    return SP_SUCCESS;
}

void sp_fir_destroy(sp_fir_filter_t* filter) {
    if (filter) {
        free(filter->coefficients);
        free(filter->buffer);
        filter->coefficients = NULL;
        filter->buffer = NULL;
        filter->length = 0;
    }
}

// Benchmarking implementation
void sp_benchmark_start(sp_benchmark_t* bench, const char* function_name) {
    if (bench) {
#ifdef __ARM_ARCH
        // Enable DWT cycle counter
        DWT_CTRL |= 1;
        bench->start_cycles = DWT_CYCCNT;
#endif
        bench->function_name = function_name;
    }
}

void sp_benchmark_end(sp_benchmark_t* bench) {
    if (bench) {
#ifdef __ARM_ARCH
        bench->end_cycles = DWT_CYCCNT;
        bench->total_cycles = bench->end_cycles - bench->start_cycles;
#endif
    }
}

void sp_benchmark_print(const sp_benchmark_t* bench) {
    if (bench) {
        // In a real implementation, you'd want to use your platform's debug output
        // This is just a placeholder
        (void)bench;  // Suppress unused parameter warning
    }
} 