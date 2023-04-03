/*
 * Copyright (c) 2018-2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
	Knowles SPH0645LM4H-B

		 returns 16-bit mono samples at configured sample rate
*/

#include "xsmc.h"
#include "mc.defines.h"
#include "mc.xs.h"

#if ESP32
	#include "freertos/FreeRTOS.h"
	#include "driver/i2s.h"
#else
	#error unsupported target
#endif

#ifndef MODDEF_AUDIOIN_SAMPLERATE
	#define MODDEF_AUDIOIN_SAMPLERATE (8000)
#endif
#ifndef MODDEF_AUDIOIN_BITSPERSAMPLE
	#define MODDEF_AUDIOIN_BITSPERSAMPLE (16)
#endif

#if ESP32
	#ifndef MODDEF_AUDIOIN_I2S_NUM
		#define MODDEF_AUDIOIN_I2S_NUM (1)
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
	#ifndef MODDEF_AUDIOIN_I2S_ADC
		#define MODDEF_AUDIOIN_I2S_ADC (0)
	#endif
#endif

void xs_audioin_destructor(void *data)
{
	if (data) {
		i2s_stop(MODDEF_AUDIOIN_I2S_NUM);

#if MODDEF_AUDIOIN_I2S_ADC
		i2s_adc_disable(MODDEF_AUDIOIN_I2S_NUM);
#endif

		i2s_driver_uninstall(MODDEF_AUDIOIN_I2S_NUM);
	}
}

void xs_audioin(xsMachine *the)
{
	esp_err_t err;

#if !MODDEF_AUDIOIN_I2S_ADC
	i2s_config_t i2s_config = {
		  mode: (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
		  sample_rate: MODDEF_AUDIOIN_SAMPLERATE,
		  bits_per_sample: I2S_BITS_PER_SAMPLE_32BIT,
		  channel_format: I2S_CHANNEL_FMT_ONLY_RIGHT,
		  communication_format: (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_MSB),
		  intr_alloc_flags: 0,
		  dma_buf_count: 4,
		  dma_buf_len: 1024
	};

#if MODDEF_AUDIOIN_I2S_PDM
	i2s_config.mode |= I2S_MODE_PDM;
#endif

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
#else
	i2s_config_t i2s_config = {
		//@@ playback modes set too!!
		  mode: (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX  | I2S_MODE_DAC_BUILT_IN | I2S_MODE_ADC_BUILT_IN),
		  sample_rate: MODDEF_AUDIOIN_SAMPLERATE,
		  bits_per_sample: I2S_BITS_PER_SAMPLE_16BIT,
		  channel_format: I2S_CHANNEL_FMT_RIGHT_LEFT,
		  communication_format: I2S_COMM_FORMAT_STAND_I2S,
		  intr_alloc_flags: 0,
		  dma_buf_count: 4,
		  dma_buf_len: 1024
	};

//	//@@ adc1_config_* may be unnecessary
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_0);

	err = i2s_driver_install(MODDEF_AUDIOIN_I2S_NUM, &i2s_config, 0, NULL);
	if (err) xsUnknownError("driver install failed");

	err = i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_6);
	if (err) xsUnknownError("set adc mode failed");

	err = i2s_adc_enable(MODDEF_AUDIOIN_I2S_NUM);
	if (err) xsUnknownError("adc enable failed");

	//@@ note: no i2s_start
#endif

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
	int argc = xsmcArgc;
	int sampleCount = xsmcToInteger(xsArg(0));
	int i, byteCount;
#if 16 == MODDEF_AUDIOIN_BITSPERSAMPLE
	int16_t *samples;

	byteCount = sampleCount * sizeof(int16_t);
#else
	int8_t *samples;

	byteCount = sampleCount * sizeof(int8_t);
#endif
	if (1 == argc) {
		xsmcSetArrayBuffer(xsResult, NULL, byteCount);
		samples = xsmcToArrayBuffer(xsResult);
	}
	else {
		int offset = (argc > 2) ? xsmcToInteger(xsArg(2)) : 0;
		void *data;
		xsUnsignedValue dataSize;

		xsmcGetBufferWritable(xsArg(1), &data, &dataSize);

		if ((offset < 0) || ((offset + byteCount) > dataSize))
			xsRangeError("invalid");

		samples = (void *)(offset + (uintptr_t)data);

		xsResult = xsArg(1);
	}

	for (i = 0; i < sampleCount; ) {
		size_t bytes_read = 0;
		int32_t buf32[64];
		int j;
		int need = sampleCount - i;
		if (need > 64)
			need = 64;

		i2s_read(MODDEF_AUDIOIN_I2S_NUM, (void *)buf32, need << 2, &bytes_read, portMAX_DELAY);
		need = bytes_read >> 2;
		for (j = 0; j < need; j++) {
#if MODDEF_AUDIOIN_I2S_ADC
#if 16 == MODDEF_AUDIOIN_BITSPERSAMPLE
			samples[i++] = (buf32[j] << 4) ^ 0x8000;
#else	/* 8-bit */
			samples[i++] = buf32[j] >> 4;
#endif
#else	/* !MODDEF_AUDIOIN_I2S_ADC */
#if 16 == MODDEF_AUDIOIN_BITSPERSAMPLE
			samples[i++] = buf32[j] >> 15;	// "The Data Format is I2S, 24 bit, 2’s compliment, MSB first. The Data Precision is 18 bits, unused bits are zeros"
#else	/* 8-bit */
			samples[i++] = (buf32[j] >> (15 + 8)) ^ 0x80;
#endif
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
