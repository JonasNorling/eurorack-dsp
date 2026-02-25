#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(analog_in);

#include "analog_in.h"
#include "leds.h"

#define ADC_NODE DT_ALIAS(adc0)
static const struct device *adc = DEVICE_DT_GET(ADC_NODE);

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_channel_cfg adc_channels[] = {
	DT_FOREACH_CHILD_SEP(ADC_NODE, ADC_CHANNEL_CFG_DT, (,))};

#define CHANNEL_COUNT ARRAY_SIZE(adc_channels)

static uint16_t buf[CHANNEL_COUNT];
struct adc_sequence sequence = {
    .buffer = buf,
    .buffer_size = sizeof(buf),
    .resolution = 12,
    .oversampling = 5,
};

/* Store floating point values for use by other modules.
 * We expect 32-bit floats to be written/read atomically
 * without any special sauce beyond "volatile". */
static volatile float values[CHANNEL_COUNT];

int analog_in_init(void)
{
	for (size_t i = 0U; i < CHANNEL_COUNT; i++) {
        sequence.channels |= BIT(adc_channels[i].channel_id);
		int err = adc_channel_setup(adc, &adc_channels[i]);
		if (err < 0) {
			printk("Could not setup channel #%d (%d)\n", i, err);
			return 1;
		}
	}

    return 0;
}

void analog_in_run(void)
{
    while (1) {
        led_set(1, true);
		int err = adc_read(adc, &sequence);
        led_set(1, false);
		if (err < 0) {
			printk("Could not read (%d)\n", err);
		}
        for (int i = 0; i < CHANNEL_COUNT; i++) {
            // Convert 12-bit to 0..1
            values[i] = (float)buf[i] / 0x1000;
        }
	}
}

float analog_in_get(int n)
{
    return values[n];
}
