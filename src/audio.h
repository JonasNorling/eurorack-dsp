#pragma once
#include <stdint.h>

typedef int16_t sample_t;
#define FRAMES_PER_BLOCK 32
#define SAMPLE_RATE 48000

typedef struct {
    sample_t s[2];  // left & right channel
} frame_t;

int audio_init(void);
int audio_run(void(*dsp_fn)(const frame_t * const in, frame_t *const out));
