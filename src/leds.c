#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "leds.h"

#define CHECK(x) if (!(x)) { printf("FAILED AT LINE %s:%d\n", __FILE__, __LINE__); return 1; }

static const struct gpio_dt_spec leds[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
};

int leds_init(void)
{
    for (int i = 0; i < ARRAY_SIZE(leds); i++) {
    	CHECK(gpio_is_ready_dt(&leds[i]));
	    CHECK(gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_ACTIVE) == 0);
    }
}

void led_set(int n, bool value)
{
    (void)gpio_pin_set_dt(&leds[n], value);
}
