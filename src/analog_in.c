#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(analog_in);

#include "analog_in.h"
#include "leds.h"

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

/* We expect 32-bit floats to be written/read atomically
 * without any special sauce beyond "volatile". */
static volatile float values[ARRAY_SIZE(adc_channels)];

int analog_in_init(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!adc_is_ready_dt(&adc_channels[i])) {
			printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
			return 1;
		}

		int err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printk("Could not setup channel #%d (%d)\n", i, err);
			return 1;
		}
	}

    return 0;
}

void analog_in_run(void)
{
    uint16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		.buffer_size = sizeof(buf),
        .oversampling = 5,
	};
    (void)adc_sequence_init_dt(&adc_channels[0], &sequence);

    while (1) {
        led_set(1, true);
		int err = adc_read_dt(&adc_channels[0], &sequence);
        led_set(1, false);
		if (err < 0) {
			printk("Could not read (%d)\n", err);
		}
        values[0] = (float)buf / 0x1000;  // Convert 12-bit to 0..1
	}
}

float analog_in_get(int n)
{
    return values[n];
}
