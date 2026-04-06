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

static int min_in[2];
static int max_in[2];
static bq_state filter_state[2];
static slope_state filter_cutoff_slope;
static float gate_energy = 0;

static inline float volume(float a)
{
	return a*a;
}

static void _dsp_do(const frame_t * const restrict in, frame_t * const restrict out)
{
	const float pot[] = {
		analog_in_get(0),
		analog_in_get(1),
		analog_in_get(2),
		analog_in_get(3),
	};
	const float cv[] = {
		CLAMP(analog_in_get(4) * 3, 0.0f, 1.0f),  // Normalize to 1.0 at 5V
		CLAMP(analog_in_get(5) * 3, 0.0f, 1.0f),  // Normalize to 1.0 at 5V
	};

	const float envelope = cv[0];
	const float trigger = cv[1];
	const float sustain = pot[2];
	const float q_factor = RAMP(pot[3], 1, 10);
	float_block_t samples[2];
	float_block_t buf[2];

	gate_energy += trigger * 0.05f;
	gate_energy *= 0.90f + sustain * 0.11f;
	gate_energy = CLAMP(gate_energy, envelope, 1.0f);

	const float gain = volume(pot[0] * 2) * volume(gate_energy);
	const float cutoff_hz = CLAMP(RAMP(volume(pot[1]), 0, 2000) + RAMP(volume(gate_energy), 0, NYQUIST), 20, NYQUIST);

	for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
		min_in[0] = min(min_in[0], in[i].s[0]);
		max_in[0] = max(max_in[0], in[i].s[0]);
		min_in[1] = min(min_in[1], in[i].s[1]);
		max_in[1] = max(max_in[1], in[i].s[1]);

		samples[0][i] = gain * in[i].s[0];
		samples[1][i] = gain * in[i].s[1];
	}
	led_set(1, will_clip(samples[0], FRAMES_PER_BLOCK) || will_clip(samples[1], FRAMES_PER_BLOCK));

	// FIXME: The y filter state is fucked up by fast cutoff changes. This makes it better but
	// I don't think the filter works as intended now.
	filter_state[0].Y[0] = filter_state[0].X[0];
	filter_state[0].Y[1] = filter_state[0].X[1];
	filter_state[1].Y[0] = filter_state[1].X[0];
	filter_state[1].Y[1] = filter_state[1].X[1];

	bq_coeffs filter_coeffs;
	const float filtered_cutoff_hz = slope_limit(&filter_cutoff_slope, NYQUIST / 5, cutoff_hz);
	bq_make_lowpass(&filter_coeffs, HZ2OMEGA(filtered_cutoff_hz), q_factor);

	bq_process(samples[0], buf[0], FRAMES_PER_BLOCK, &filter_coeffs, &filter_state[0]);
	bq_process(samples[1], buf[1], FRAMES_PER_BLOCK, &filter_coeffs, &filter_state[1]);
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
		LOG_INF("Filter L: %.2f %.2f %.2f %.2f", (double)filter_state[0].X[0], (double)filter_state[0].X[1], (double)filter_state[0].Y[0], (double)filter_state[0].Y[1]);
		LOG_INF("Filter R: %.2f %.2f %.2f %.2f", (double)filter_state[1].X[0], (double)filter_state[1].X[1], (double)filter_state[1].Y[0], (double)filter_state[1].Y[1]);
	}
}
