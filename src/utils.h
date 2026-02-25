#pragma once
#include <stdint.h>

#define NYQUIST (SAMPLE_RATE / 2)

/**
 * Convert a frequency to radians for making filters
 */
#define HZ2OMEGA(f) ((f) * (3.1415926f / NYQUIST))

#define CLAMP(v, min, max) (v) > (max) ? (max) : ((v) < (min) ? (min) : (v))
#define RAMP(v, start, end) ((end) * (v) + (start * (1.0f-(v))))

static inline int16_t saturate(int16_t v)
{
    return CLAMP(v, INT16_MIN, INT16_MAX);
}

static inline float saturate_tube(float _x)
{
    // Clippy, tube-like distortion
    // References : Posted by Partice Tarrabia and Bram de Jong
    const float depth = 0.5;
    const float k = 2.0f * depth / (1.0f - depth);
    const float b = INT16_MAX;
    const float x = _x / b;
    const float ret = b * (1 + k) * x / (1 + k * fabsf(x));
    if (ret < INT16_MIN) return INT16_MIN;
    if (ret > INT16_MAX) return INT16_MAX;
    return ret;
}
