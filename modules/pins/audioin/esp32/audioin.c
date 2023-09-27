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
	#include "driver/i2s_std.h"
	#include "driver/i2s_pdm.h"
	#include "esp_adc/adc_continuous.h"
	#include "soc/soc_caps.h"
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
	#ifndef MODDEF_AUDIOIN_I2S_ADC
		#define MODDEF_AUDIOIN_I2S_ADC (0)
	#endif

	#if MODDEF_AUDIOIN_SAMPLERATE < SOC_ADC_SAMPLE_FREQ_THRES_LOW
		#error "sample rate too low"
	#elif MODDEF_AUDIOIN_SAMPLERATE > SOC_ADC_SAMPLE_FREQ_THRES_HIGH
		#error "sample rate too high"
	#endif
#endif

#define kADCSampleLength (256)

void xs_audioin_destructor(void *data)
{
	if (data) {
#if MODDEF_AUDIOIN_I2S_ADC
		adc_continuous_handle_t adcHandle = data;
		adc_continuous_stop(adcHandle);
		adc_continuous_deinit(adcHandle);
#else
		i2s_chan_handle_t rx_handle = data;
		i2s_channel_disable(rx_handle);
		i2s_del_channel(rx_handle);
#endif
	}
}

void xs_audioin(xsMachine *the)
{
#if MODDEF_AUDIOIN_I2S_ADC
	adc_continuous_handle_t adcHandle;
	adc_continuous_handle_cfg_t adc_config = {
		.max_store_buf_size = 1024,
		.conv_frame_size = kADCSampleLength,
	};
	ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &adcHandle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = MODDEF_AUDIOIN_SAMPLERATE,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
    };

	adc_digi_pattern_config_t adc_pattern[1] = {0};
	adc_pattern[0].atten = ADC_ATTEN_DB_11;		// if this is ADC_ATTEN_DB_0 M5Stack Fire is overdriven like crazy
	adc_pattern[0].channel = ADC_CHANNEL_6;		//@@ config
	adc_pattern[0].unit = ADC_UNIT_1;				//@@ config
	adc_pattern[0].bit_width = ADC_BITWIDTH_12;
	dig_cfg.pattern_num = 1;
	dig_cfg.adc_pattern = adc_pattern;
	ESP_ERROR_CHECK(adc_continuous_config(adcHandle, &dig_cfg));

	ESP_ERROR_CHECK(adc_continuous_start(adcHandle));
	xsmcSetHostData(xsThis, (void *)adcHandle);
#else
	esp_err_t err;
	i2s_chan_handle_t rx_handle;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(MODDEF_AUDIOIN_I2S_NUM, I2S_ROLE_MASTER);

    err = i2s_new_channel(&chan_cfg, NULL, &rx_handle);
	if (err) xsUnknownError("i2s_new_channel failed");

#if MODDEF_AUDIOIN_I2S_PDM
    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(MODDEF_AUDIOIN_SAMPLERATE),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = MODDEF_AUDIOIN_I2S_LR_PIN,
            .din = MODDEF_AUDIOIN_I2S_DATAIN,
            .invert_flags = {
                .clk_inv = false,
            }
        }
    };

    pdm_rx_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;

    err = i2s_channel_init_pdm_rx_mode(rx_handle, &pdm_rx_cfg);
	if (err) {
		i2s_del_channel(rx_handle);
		xsUnknownError("i2s_channel_init_pdm_rx_mode failed");
	}
#else
	#error STD untested

    i2s_std_config_t rx_std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(MODDEF_AUDIOIN_SAMPLERATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = MODDEF_AUDIOIN_I2S_BCK_PIN,
            .ws   = MODDEF_AUDIOIN_I2S_LR_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din  = MODDEF_AUDIOIN_I2S_DATAIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            }
        }
    };
    rx_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;		//@@

    err = i2s_channel_init_std_mode(rx_handle, &rx_std_cfg);
	if (err) {
		i2s_del_channel(rx_handle);
		xsUnknownError("i2s_channel_init_std_mode failed");
	}
#endif

    err = i2s_channel_enable(rx_handle);
	if (err) {
		i2s_del_channel(rx_handle);
		xsUnknownError("i2s_channel_enable failed");
	}

	xsmcSetHostData(xsThis, (void *)rx_handle);
#endif
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
#if MODDEF_AUDIOIN_I2S_ADC
	adc_continuous_handle_t adcHandle = xsmcGetHostData(xsThis);
#else
	i2s_chan_handle_t rx_handle = xsmcGetHostData(xsThis);
#endif
	esp_err_t err;
	int argc = xsmcArgc;
	size_t bytes_read;
	int sampleCount = xsmcToInteger(xsArg(0));
	void *samples;
	int byteCount = (sampleCount * MODDEF_AUDIOIN_BITSPERSAMPLE) >> 3;

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

#if MODDEF_AUDIOIN_I2S_ADC
	uint8_t input[kADCSampleLength];

	while (sampleCount) {
		uint32_t use = sizeof(input);
		uint32_t needed = sampleCount * sizeof(adc_digi_output_data_t);
		if (use > needed) use = needed;
		uint32_t used;

      err = adc_continuous_read(adcHandle, input, use, &used, 17);
		if (err) break;

		used /= sizeof(adc_digi_output_data_t);
		sampleCount -= used;

		adc_digi_output_data_t *in = (adc_digi_output_data_t *)input;
#if 8 == MODDEF_AUDIOIN_BITSPERSAMPLE
		uint8_t *out = (uint8_t *)samples;
		while (used--) {
			*out++ = in->type1.data >> 4;
			in++;
		}
		samples = (void *)out;
#else // 16 == MODDEF_AUDIOIN_BITSPERSAMPLE
		uint16_t *out = (uint16_t *)samples;
		while (used--) {
			*out++ = (in->type1.data - 2048) * 8;
			in++;
		}
		samples = (void *)out;
#endif
	}
#elif MODDEF_AUDIOIN_I2S_PDM && (8 == MODDEF_AUDIOIN_BITSPERSAMPLE)
	int8_t *out = samples;
	int i;
	while (sampleCount) {
		const int tmpCount = 512;
		int16_t tmp[tmpCount];
		int use = sampleCount;
		if (use > tmpCount)
			use = tmpCount;
		err = i2s_channel_read(rx_handle, (char *)tmp, use << 1, &bytes_read, portMAX_DELAY);
		if (err) break;
	
		for (i = 0; i < use; i++) 
			*out++ = (tmp[i] >> 8) ^ 0x80;
		sampleCount -= use;
	}
#else
	err = i2s_channel_read(rx_handle, (char *)samples, byteCount, &bytes_read, portMAX_DELAY);
#endif
	if (err) xsUnknownError("i2s_channel_read failed");
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
