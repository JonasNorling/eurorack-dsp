#include <stdint.h>
#include <math.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dsp);

#include "dsp.h"
#include "analog_in.h"
#include "biquad.h"
#include "perftimer.h"
#include "utils.h"
#include "leds.h"

typedef float float_block_t[FRAMES_PER_BLOCK];
#define TAU (2 * 3.14159265358979323846)

static int min_in[2];
static int max_in[2];

static inline float volume(float a)
{
	return a*a;
}

static void _dsp_do(const frame_t * const restrict in, frame_t * const restrict out)
{
	static double sin_phase = 0.0;
	const double sin_volume = volume(analog_in_get(0));
	const float input_volume = volume(analog_in_get(1) * 3) + volume(analog_in_get(4) * 3);
	const float cutoff_hz = RAMP(volume(analog_in_get(2)), 10, 2000) + RAMP(volume(analog_in_get(5)), 0, 10000);
	const float q_factor = RAMP(analog_in_get(3), 1, 10);
	float_block_t float_samples[2];
	float_block_t buf[2];

	for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
		min_in[0] = min(min_in[0], in[i].s[0]);
		max_in[0] = max(max_in[0], in[i].s[0]);
		min_in[1] = min(min_in[1], in[i].s[1]);
		max_in[1] = max(max_in[1], in[i].s[1]);

		sin_phase += cutoff_hz * TAU / SAMPLE_RATE;
		if (sin_phase > TAU) {
			sin_phase -= TAU;
		}
		float_samples[0][i] = sin_volume * 0x7fff * sin(sin_phase);
		float_samples[1][i] = float_samples[0][i];

		if (input_volume < 1.0f) {
			float_samples[0][i] += input_volume * in[i].s[0];
			float_samples[1][i] += input_volume * in[i].s[1];
		} else {
			// Overdrive with distortion
			float_samples[0][i] += saturate_tube(input_volume * in[i].s[0]);
			float_samples[1][i] += saturate_tube(input_volume * in[i].s[1]);
		}
	}
	led_set(1, will_clip(float_samples[0], FRAMES_PER_BLOCK) || will_clip(float_samples[1], FRAMES_PER_BLOCK));

	bq_coeffs filter_coeffs;
	static bq_state filter_state[2];
	bq_make_lowpass(&filter_coeffs, HZ2OMEGA(cutoff_hz), q_factor);
	bq_process(float_samples[0], buf[0], FRAMES_PER_BLOCK, &filter_coeffs, &filter_state[0]);
	bq_process(float_samples[1], buf[1], FRAMES_PER_BLOCK, &filter_coeffs, &filter_state[1]);
	led_set(3, will_clip(buf[0], FRAMES_PER_BLOCK) || will_clip(buf[1], FRAMES_PER_BLOCK));

	for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
		out[i].s[0] = float_to_i16(buf[0][i]);
		out[i].s[1] = float_to_i16(buf[1][i]);
	}
}

void dsp_do(const frame_t * const restrict in, frame_t * const restrict out)
{
	static perftimer_t pt;
	perftimer_start(&pt);
	_dsp_do(in, out);
	perftimer_end(&pt);

	if (pt.calls == 1000) {
		perftimer_report("dsp", pt);
		pt = (perftimer_t){};
		LOG_INF("Analog in: %4d %4d %4d %4d %4d %4d",
			(int)(analog_in_get(0) * 1000),
			(int)(analog_in_get(1) * 1000),
			(int)(analog_in_get(2) * 1000),
			(int)(analog_in_get(3) * 1000),
			(int)(analog_in_get(4) * 1000),
			(int)(analog_in_get(5) * 1000)
		);
		LOG_INF("Input range: %d-%d, %d-%d", min_in[0], max_in[0], min_in[1], max_in[1]);
		min_in[0] = 0;
		min_in[1] = 0;
		max_in[0] = 0;
		max_in[1] = 0;
	}
}
