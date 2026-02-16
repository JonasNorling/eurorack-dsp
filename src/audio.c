#include "audio.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio);

#define BYTES_PER_SAMPLE sizeof(sample_t)
#define SAMPLES_PER_BLOCK (FRAMES_PER_BLOCK * 2)
#define BLOCK_SIZE  (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)
#define BLOCK_COUNT 4
K_MEM_SLAB_DEFINE_IN_SECT_STATIC(mem_slab, __nocache, BLOCK_SIZE, BLOCK_COUNT, 4);

static const struct device *const i2s_dev_rx = DEVICE_DT_GET(DT_ALIAS(i2s_codec_rx));
static const struct device *const i2s_dev_tx = DEVICE_DT_GET(DT_ALIAS(i2s_codec_tx));

#define I2C_ADDR 0x0a

/**
 * Configure NXP SGTL5000 codec
 */
static int config_codec(void)
{
	const struct device *const i2c_dev = DEVICE_DT_GET(DT_ALIAS(i2c_codec));
	int ret;

	if (!device_is_ready(i2c_dev)) {
		printk("%s is not ready\n", i2c_dev->name);
		return false;
	}

	uint16_t value = 0;
	uint16_t reg_addr = 0;
	ret = i2c_write_read(i2c_dev, I2C_ADDR, &reg_addr, 2, &value, 2);
	if (ret != 0) {
		LOG_ERR("Codec config failure on line %d", __LINE__);
		return 1;
	}
	LOG_INF("SGTL5000 chip id = %x", (unsigned)value);

	/*
	 * Values to program in registers.
	 * Mostly based on control_sgtl5000 from Teensy Audio lib.
	 * The Teensy audio adapter board is straight forward:
	 *   VDDD 1.8V supplied by an LDO
	 *   VDDIO 3.3V from Teensy
	 *   VDDA 3.3V from Teensy
	 */
	static const uint16_t incantations[][2] = {
		{ 0x0030, 0x4260 }, // CHIP_ANA_POWER: external 1.8V VDDD
		{ 0x0030, 0x4a60 }, // CHIP_ANA_POWER: enable charge pump
		{ 0x0026, 0x006c }, // CHIP_LINREG_CTRL: VDDA and VDDIO >3.1V
		{ 0x0028, 0x01f2 }, // CHIP_REF_CTRL: Bandgap reference settings
		{ 0x002c, 0x0f22 }, // CHIP_LINE_OUT_CTRL
		{ 0x003c, 0x4446 }, // CHIP_SHORT_CTRL: headphone short detector
		{ 0x0024, 0x0133 }, // CHIP_ANA_CTRL
		{ 0x0030, 0x6aff }, // CHIP_ANA_POWER: power it up
		{ 0x0002, 0x0073 }, // CHIP_DIG_POWER: power it up
		{ 0x002e, 0x1d1d }, // CHIP_LINE_OUT_VOL 1.3 Vpp
		{ 0x0004, 0x0008 }, // CHIP_CLK_CTRL: 48 kHz 256*Fs
		{ 0x0006, 0x0030 }, // CHIP_I2S_CTRL: 16 bit
		{ 0x0022, 0x1818 }, // CHIP_ANA_HP_CTRL: volume low
		{ 0x0024, 0x0026 }, // CHIP_ANA_CTRL: unmute headphones, line, adc
		{ 0x0010, 0x3c3c }, // CHIP_DAC_VOL: DAC gain 0dB (default)
		{ 0x000e, 0x0000 }, // CHIP_ADCDAC_CTRL: unmute DACs
	};

	for (int i = 0; i < ARRAY_SIZE(incantations); i++) {
		uint8_t data[4] = {
			incantations[i][0] >> 8,
			incantations[i][0] & 0xff,
			incantations[i][1] >> 8,
			incantations[i][1] & 0xff,
		};
		ret = i2c_write(i2c_dev, data, sizeof(data), I2C_ADDR);
		if (ret != 0) {
			LOG_ERR("Codec config failure on line %d", __LINE__);
			return 1;
		}
	}

	for (int i = 0; i < 0x40; i += 2) {
		uint8_t value[2] = {};
		uint8_t reg_addr[2] = {0, i};
		ret = i2c_write_read(i2c_dev, I2C_ADDR, &reg_addr, 2, &value, 2);
		if (ret != 0) {
			LOG_ERR("Codec config failure on line %d", __LINE__);
			return 1;
		}
		LOG_INF("Reg %02x = %02x%02x", i, value[0], value[1]);
	}

	return 0;
}

int audio_init(void)
{
	int ret;
    
	ret = config_codec();
	if (ret != 0) {
		LOG_ERR("Codec config failure");
		return 1;
	}

	if (!device_is_ready(i2s_dev_rx)) {
		LOG_ERR("%s is not ready\n", i2s_dev_rx->name);
		return 1;
	}

	if (!device_is_ready(i2s_dev_tx)) {
		LOG_ERR("%s is not ready\n", i2s_dev_tx->name);
		return 1;
	}

	// Use the receiver bit clock and frame sync:
	//   configure rx for async operation
	//   configure tx for sync operation
	static const struct i2s_config config = {
        .word_size = 16,
		.channels = 2,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER,
		.frame_clk_freq = SAMPLE_RATE,
		.mem_slab = &mem_slab,
		.block_size = BLOCK_SIZE,
		.timeout = 2000,
    };
	ret = i2s_configure(i2s_dev_rx, I2S_DIR_RX, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure rx stream: %d\n", ret);
		return false;
	}
	ret = i2s_configure(i2s_dev_tx, I2S_DIR_TX, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure tx stream: %d\n", ret);
		return false;
	}

	return 0;
}

int audio_run(void(*dsp_fn)(const frame_t * const in, frame_t *const out))
{
	int ret;

	LOG_INF("Starting I2S streaming");
	static frame_t out_data[FRAMES_PER_BLOCK];
	static frame_t in_data[FRAMES_PER_BLOCK];
	ret = i2s_buf_write(i2s_dev_tx, out_data, sizeof(out_data));
	if (ret < 0) {
		LOG_ERR("Failed to write data: %d\n", ret);
		return 1;
	}
	ret = i2s_buf_write(i2s_dev_tx, out_data, sizeof(out_data));
	if (ret < 0) {
		LOG_ERR("Failed to write data: %d\n", ret);
		return 1;
	}

	ret = i2s_trigger(i2s_dev_rx, I2S_DIR_RX, I2S_TRIGGER_START);
	if (ret < 0) {
		LOG_ERR("Failed to start rx: %d\n", ret);
		return 1;
	}
	ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret < 0) {
		LOG_ERR("Failed to start tx: %d\n", ret);
		return 1;
	}

	while (1) {
		size_t len = 0;
		ret = i2s_buf_read(i2s_dev_rx, in_data, &len);
		if (ret < 0 || len != sizeof(in_data)) {
			LOG_ERR("Failed to read data: %d\n", ret);
			break;
		}

		dsp_fn(in_data, out_data);

		ret = i2s_buf_write(i2s_dev_tx, out_data, sizeof(out_data));
		if (ret < 0) {
			LOG_ERR("Failed to write data: %d\n", ret);
			break;
		}
	}

    return 0;
}
