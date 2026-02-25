#include <stdint.h>
#include <math.h>

#include "dsp.h"
#include "analog_in.h"
#include "biquad.h"
#include "utils.h"

typedef float float_block_t[FRAMES_PER_BLOCK];

static inline float volume(float a)
{
	return a*a;
}

void dsp_do(const frame_t * const restrict in, frame_t * const restrict out)
{
	static unsigned sample_counter = 0;
	const float sin_volume = volume(analog_in_get(0));
	const float input_volume = volume(analog_in_get(1) * 3);
	const float cutoff_hz = RAMP(analog_in_get(2), 100, 2000);
	float_block_t float_samples[2];
	float_block_t buf[2];

	for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
		float_samples[0][i] = sin_volume * 0x7fff * sinf(220 * 6.28 * sample_counter / SAMPLE_RATE);
		float_samples[1][i] = sin_volume * 0x7fff * sinf(230 * 6.28 * sample_counter / SAMPLE_RATE);
		sample_counter++;

		if (input_volume < 1.0f) {
			float_samples[0][i] += input_volume * in[i].s[0];
			float_samples[1][i] += input_volume * in[i].s[1];
		} else {
			// Overdrive with distortion
			float_samples[0][i] += saturate_tube(input_volume * in[i].s[0]);
			float_samples[1][i] += saturate_tube(input_volume * in[i].s[1]);
		}
	}

	bq_coeffs filter_coeffs;
	static bq_state filter_state[2];
	bq_make_lowpass(&filter_coeffs, HZ2OMEGA(cutoff_hz), 1.0);
	bq_process(float_samples[0], buf[0], FRAMES_PER_BLOCK, &filter_coeffs, &filter_state[0]);
	bq_process(float_samples[1], buf[1], FRAMES_PER_BLOCK, &filter_coeffs, &filter_state[1]);

	for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
		out[i].s[0] = saturate(buf[0][i]);
		out[i].s[1] = saturate(buf[1][i]);
	}
}
