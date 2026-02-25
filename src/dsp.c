#include <stdint.h>
#include <math.h>

#include "dsp.h"
#include "analog_in.h"

static inline float volume(float a)
{
	return a*a;
}

void dsp_do(const frame_t * const restrict in, frame_t * const restrict out)
{
	static unsigned sample_counter = 0;
	const float sin_volume = volume(analog_in_get(0));
	const float input_volume = volume(analog_in_get(1));

	for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
		int32_t o[2];
		o[0] = sin_volume * 0x7fff * sinf(220 * 6.28 * sample_counter / SAMPLE_RATE);
		o[1] = sin_volume * 0x7fff * sinf(230 * 6.28 * sample_counter / SAMPLE_RATE);
		sample_counter++;

		o[0] += input_volume * in[i].s[0];
		o[1] += input_volume * in[i].s[1];

		// FIXME: Clamp/overflow nicelier
		out[i].s[0] = o[0];
		out[i].s[1] = o[1];
	}
}
