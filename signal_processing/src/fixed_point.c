#include "fixed_point.h"
#include <math.h>

fixed_point_t sp_float_to_fixed(float value) {
    float scaled = value * (1 << FIXED_POINT_BITS);
    if (scaled > INT32_MAX) return INT32_MAX;
    if (scaled < INT32_MIN) return INT32_MIN;
    return (fixed_point_t)scaled;
}

float sp_fixed_to_float(fixed_point_t value) {
    return ((float)value) / (1 << FIXED_POINT_BITS);
} 