#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#include <stdint.h>

// Fixed point format (Q1.31)
typedef int32_t fixed_point_t;
#define FIXED_POINT_BITS 31

// Convert between float and fixed point
fixed_point_t sp_float_to_fixed(float value);
float sp_fixed_to_float(fixed_point_t value);

#endif // FIXED_POINT_H 