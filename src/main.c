#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include "audio.h"

#define CHECK(x) if (!(x)) { printf("FAILED AT LINE %s:%d\n", __FILE__, __LINE__); return 1; }

/* From adc_dt demo */
#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
};

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
	// Wait for USB console to connect
	k_msleep(2000);
	LOG_INF("Hello there!\n");

	CHECK(gpio_is_ready_dt(&led));
	CHECK(gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) == 0);

	// Activate the two pins beside 21 (ADC input) so we can stick a pot there
	// teensy pin 20 = AD_B1_10 = gpio 1.26
	CHECK(gpio_pin_configure(DEVICE_DT_GET(DT_NODELABEL(gpio1)), 26, GPIO_OUTPUT_INACTIVE) == 0);
	// teensy pin 22 = AD_B1_08 = gpio 1.24
	CHECK(gpio_pin_configure(DEVICE_DT_GET(DT_NODELABEL(gpio1)), 24, GPIO_OUTPUT_ACTIVE) == 0);

	uint16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		.buffer_size = sizeof(buf),
	};

	/* Configure ADC channels */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!adc_is_ready_dt(&adc_channels[i])) {
			printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
			return 0;
		}

		int err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printk("Could not setup channel #%d (%d)\n", i, err);
			return 0;
		}
	}

	CHECK(audio_init() == 0);

	while (1) {
		(void)gpio_pin_toggle_dt(&led);

		(void)adc_sequence_init_dt(&adc_channels[0], &sequence);
		int err = adc_read_dt(&adc_channels[0], &sequence);
		if (err < 0) {
			printk("Could not read (%d)\n", err);
		}
		char bar[65] = "";
		memset(bar, '=', 64 * buf / 0x1000);
		printf("ADC value: %04x %s\n", (int)buf, bar);

		k_msleep(100);
	}
	return 0;
}
