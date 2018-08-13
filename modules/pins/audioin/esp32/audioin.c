/*
	Knowles SPH0645LM4H-B

		 returns 16-bit mono samples at configured sample rate
*/

#include "xsmc.h"
#include "mc.defines.h"

#if ESP32
	#include "freertos/FreeRTOS.h"
	#include "driver/i2s.h"
#else
	#error unsupported target
#endif

#if ESP32
	#ifndef MODDEF_AUDIOIN_I2S_NUM
		#define MODDEF_AUDIOIN_I2S_NUM (0)
	#endif
	#ifndef MODDEF_AUDIOIN_I2S_BCK_PIN
		#define MODDEF_AUDIOIN_I2S_BCK_PIN (32)
	#endif
	#ifndef MODDEF_AUDIOIN_I2S_LR_PIN
		#define MODDEF_AUDIOIN_I2S_LR_PIN (33)
	#endif
	#ifndef MODDEF_AUDIOIN_I2S_DATAIN
		#define MODDEF_AUDIOIN_I2S_DATAIN (27)
	#endif
#endif

#ifndef MODDEF_AUDIOIN_SAMPLERATE
	#define MODDEF_AUDIOIN_SAMPLERATE (8000)
#endif
#ifndef MODDEF_AUDIOIN_BITSPERSAMPLE
	#define MODDEF_AUDIOIN_BITSPERSAMPLE (16)
#endif

void xs_audioin_destructor(void *data)
{
	if (data)
		i2s_stop(MODDEF_AUDIOIN_I2S_NUM);
}

void xs_audioin(xsMachine *the)
{
	esp_err_t err;

	i2s_config_t i2s_config = {
		  mode: (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
		  sample_rate: MODDEF_AUDIOIN_SAMPLERATE,
		  bits_per_sample: I2S_BITS_PER_SAMPLE_32BIT,
		  channel_format: I2S_CHANNEL_FMT_ONLY_RIGHT,
		  communication_format: (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB),
		  intr_alloc_flags: ESP_INTR_FLAG_LEVEL1,
		  dma_buf_count: 4,
		  dma_buf_len: 1024
	};

	i2s_pin_config_t pin_config = {
		.bck_io_num = MODDEF_AUDIOIN_I2S_BCK_PIN,
		.ws_io_num = MODDEF_AUDIOIN_I2S_LR_PIN,
		.data_out_num = I2S_PIN_NO_CHANGE,
		.data_in_num = MODDEF_AUDIOIN_I2S_DATAIN
	};

	err = i2s_driver_install(MODDEF_AUDIOIN_I2S_NUM, &i2s_config, 0, NULL);
	if (err) xsUnknownError("driver install failed");

	err = i2s_set_pin(MODDEF_AUDIOIN_I2S_NUM, &pin_config);
	if (err) xsUnknownError("pin config failed");

	err = i2s_start(MODDEF_AUDIOIN_I2S_NUM);
	if (err) xsUnknownError("i2s start failed");

	xsmcSetHostData(xsThis, (void *)-1);
}

void xs_audioin_close(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	if (!data)
		return;
	xs_audioin_destructor(data);
	xsmcSetHostData(xsThis, NULL);
}

void xs_audioin_read(xsMachine *the)
{
	int sampleCount = xsmcToInteger(xsArg(0));
	int i, byteCount;
#if 16 == MODDEF_AUDIOIN_BITSPERSAMPLE
	int16_t *samples;

	byteCount = sampleCount * sizeof(int16_t);
#else
	int8_t *samples;

	byteCount = sampleCount * sizeof(int8_t);
#endif
	xsmcSetArrayBuffer(xsResult, NULL, byteCount);
	samples = xsmcToArrayBuffer(xsResult);

	for (i = 0; i < sampleCount; ) {
		int32_t buf32[64];
		int j;
		int need = sampleCount - i;
		if (need > 64)
			need = 64;

		i2s_read_bytes(MODDEF_AUDIOIN_I2S_NUM, (void *)buf32, sizeof(buf32), portMAX_DELAY);
		for (j = 0; j < need; j++) {
#if 16 == MODDEF_AUDIOIN_BITSPERSAMPLE
			samples[i++] = buf32[j] >> 15;	// "The Data Format is I2S, 24 bit, 2’s compliment, MSB first. The Data Precision is 18 bits, unused bits are zeros"
#else
			samples[i++] = (buf32[j] >> (15 + 8)) ^ 0x80;
#endif
		}
	}
}

void xs_audioin_get_sampleRate(xsMachine *the)
{
	xsmcSetInteger(xsResult, MODDEF_AUDIOIN_SAMPLERATE);
}

void xs_audioin_get_bitsPerSample(xsMachine *the)
{
	xsmcSetInteger(xsResult, MODDEF_AUDIOIN_BITSPERSAMPLE);
}

void xs_audioin_get_numChannels(xsMachine *the)
{
	xsmcSetInteger(xsResult, 1);
}
