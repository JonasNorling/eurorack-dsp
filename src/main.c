#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include "analog_in.h"
#include "audio.h"
#include "dsp.h"
#include "leds.h"

#define CHECK(x) if (!(x)) { printf("FAILED AT LINE %s:%d\n", __FILE__, __LINE__); return 1; }

static const int thread_prio_audio = 0;
static const int thread_prio_analog_in = 1;

static struct k_thread audio_thread;
static K_THREAD_STACK_DEFINE(audio_stack, 1024);
static struct k_thread analog_in_thread;
static K_THREAD_STACK_DEFINE(analog_in_stack, 1024);

static void audio_thread_entry(void *dsp_fn, void*, void*) { audio_run(dsp_fn); }
static void analog_in_thread_entry(void*, void*, void*) { analog_in_run(); }

int main(void)
{
	// Wait for USB console to connect
	k_msleep(4000);
	LOG_INF("Hello there!\n");

	CHECK(leds_init() == 0);
	CHECK(analog_in_init() == 0);
	CHECK(audio_init() == 0);

	CHECK(k_thread_create(
		&audio_thread,
		audio_stack,
		K_THREAD_STACK_SIZEOF(audio_stack),
		audio_thread_entry,
		dsp_do,
		NULL,
		NULL,
		thread_prio_audio,
		0,
		K_NO_WAIT
	));

	CHECK(k_thread_create(
		&analog_in_thread,
		analog_in_stack,
		K_THREAD_STACK_SIZEOF(analog_in_stack),
		analog_in_thread_entry,
		dsp_do,
		NULL,
		NULL,
		thread_prio_analog_in,
		0,
		K_NO_WAIT
	));

	while (1) {
		led_set(0, true);
		k_msleep(10);
		led_set(0, false);
		k_msleep(500);
	}
	return 0;
}
