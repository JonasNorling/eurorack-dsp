#include <stdint.h>
#include <math.h>

#include "dsp.h"

void dsp_do(const frame_t * const restrict in, frame_t * const restrict out)
{
	static unsigned sample_counter = 0;
	for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
		out[i].s[0] = 0x7fff * sin(220 * 6.28 * sample_counter / SAMPLE_RATE);
		out[i].s[1] = 0x7fff * sin(230 * 6.28 * sample_counter / SAMPLE_RATE);
		sample_counter++;
	}
}
