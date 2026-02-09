#include "audio.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio);


#define BYTES_PER_SAMPLE 2
#define SAMPLES_PER_BLOCK 1024
#define BLOCK_SIZE  (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)
#define BLOCK_COUNT 16
K_MEM_SLAB_DEFINE_IN_SECT_STATIC(mem_slab, __nocache, BLOCK_SIZE, BLOCK_COUNT, 4);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int audio_init(void)
{
    const struct device *const i2s_dev_codec = DEVICE_DT_GET(DT_ALIAS(i2s_codec_tx));
    
	if (!device_is_ready(i2s_dev_codec)) {
		LOG_ERR("%s is not ready\n", i2s_dev_codec->name);
		return 1;
	}

	static const struct i2s_config config = {
        .word_size = 16,
		.channels = 2,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER,
		.frame_clk_freq = 16000,
		.mem_slab = &mem_slab,
		.block_size = BLOCK_SIZE,
		.timeout = 2000,
    };
	int ret = i2s_configure(i2s_dev_codec, I2S_DIR_TX, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure codec stream: %d\n", ret);
		return false;
	}

	LOG_INF("Starting I2S streaming");
	static uint16_t data[SAMPLES_PER_BLOCK];
	ret = i2s_buf_write(i2s_dev_codec, data, BLOCK_SIZE);
	if (ret < 0) {
		LOG_ERR("Failed to write data: %d\n", ret);
		return 1;
	}

	i2s_trigger(i2s_dev_codec, I2S_DIR_TX, I2S_TRIGGER_START);

	while (1) {
		for (int i = 0; i < SAMPLES_PER_BLOCK; i++) {
			data[i] = 0x00aa;
		}
		ret = i2s_buf_write(i2s_dev_codec, data, BLOCK_SIZE);
		if (ret < 0) {
			LOG_ERR("Failed to write data: %d\n", ret);
			break;
		}
		(void)gpio_pin_toggle_dt(&led);
	}

    return 0;
}
