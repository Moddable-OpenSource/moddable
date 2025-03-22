/*
 * Copyright (c) 2019-2025 Moddable Tech, Inc.
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
	Analog - using ESP-IDF
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"			// esp platform support
#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

//#define V_REF 1100

struct AttenuationRecord {
	uint16_t atten;
	uint16_t Vmin;
	uint16_t Vmax;
};
typedef struct AttenuationRecord AttenuationRecord;
typedef struct AttenuationRecord const *Attenuation;

#define maxAtten (4)
static const AttenuationRecord kAttenuationTable[maxAtten] = {
	{ ADC_ATTEN_DB_0, 100, 950 },
	{ ADC_ATTEN_DB_2_5, 100, 1250 },
	{ ADC_ATTEN_DB_6, 150, 1750 },
	{ ADC_ATTEN_DB_12, 150, 2450 }
};


struct AnalogRecord {
	xsSlot		obj;
	uint32_t	pin;
	uint8_t 	port;
	uint8_t		channel;
	int8_t		calib;
	Attenuation	attenuation;
	adc_oneshot_unit_handle_t handle;
};
typedef struct AnalogRecord AnalogRecord;
typedef struct AnalogRecord *Analog;

#if kCPUESP32C3
	// From espressif doc:
	//  "ADC2 oneshot mode is no longer supported, due to hardware limitation"
	#define ADC_RESOLUTION (12)
	#define ADC_WIDTH 		ADC_BITWIDTH_12
	#define ADC_PORTS		(1)
	#define ADC_CHANNEL_MAX (5)

	static const uint8_t gADCMap[ADC_CHANNEL_MAX] = {		// ADC1 channel to GPIO
		0,
		1,
		2,
		3,
		4
	};
#elif kCPUESP32C6 || kCPUESP32H2
	// From espressif doc:
	#define ADC_RESOLUTION (12)
	#define ADC_WIDTH 		ADC_BITWIDTH_12
	#define ADC_PORTS		(1)
	#define ADC_CHANNEL_MAX (7)

	static const uint8_t gADCMap[ADC_CHANNEL_MAX] = {		// ADC channel to GPIO
		0,
		1,
		2,
		3,
		4,
		5,
		6
	};

#elif kCPUESP32S2 || kCPUESP32S3

#define ADC_PORTS	(2)
#if kCPUESP32S2
	#define ADC_RESOLUTION (13)
	#define ADC_WIDTH 		ADC_BITWIDTH_13
#elif kCPUESP32S3
	#define ADC_RESOLUTION (12)
	#define ADC_WIDTH 		ADC_BITWIDTH_12
#endif

	static const uint8_t gADCMap[SOC_ADC_CHANNEL_NUM(0)] = {		// ADC1 channel to GPIO
		1,
		2,
		3,
		4,
		5,
		6,
		7,
		8,
		9,
		10
	};

	static const uint8_t gADC2Map[SOC_ADC_CHANNEL_NUM(1)] = {		// ADC2 channel to GPIO
		11,
		12,
		13,
		14,
		15,
		16,
		17,
		18,
		19,
		20
	};

#else
	#define ADC_PORTS		(2)
	#define ADC_RESOLUTION (12)
	#define ADC_WIDTH 		ADC_BITWIDTH_12

	static const uint8_t gADCMap[SOC_ADC_CHANNEL_NUM(0)] = {		// ADC1 channel to GPIO
		36,
		37,
		38,
		39,
		32,
		33,
		34,
		35
	};

	static const uint8_t gADC2Map[SOC_ADC_CHANNEL_NUM(1)] = {		// ADC2 channel to GPIO
		4,
		0,
		2,
		15,
		13,
		12,
		14,
		27,
		25,
		26
	};
#endif

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED || ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED

static adc_cali_handle_t gADC1_cali_handle;
static int gADC1_caliCount = 0;
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
	static adc_cali_curve_fitting_config_t gADC1_cali_config;
#else
	static adc_cali_line_fitting_config_t gADC1_cali_config;
#endif

#if ADC_PORTS > (1)
	static adc_cali_handle_t  gADC2_cali_handle;
	static int gADC2_caliCount = 0;
	#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
		static adc_cali_curve_fitting_config_t gADC2_cali_config;
	#else
		static adc_cali_line_fitting_config_t gADC2_cali_config;
	#endif
#endif

#endif

void xs_analog_constructor_(xsMachine *the)
{
	Analog analog;
	int8_t i, channel = -1;
	int8_t port = -1, calib = 1;
	uint32_t pin;
	esp_err_t err;
	Attenuation attenuation = &kAttenuationTable[3];

	xsmcVars(3);
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = builtinGetPin(the, &xsVar(0));

    if (!builtinIsPinFree(pin))
		xsRangeError("in use");

	if (xsmcHas(xsArg(0), xsID_esp32)) {
		xsmcGet(xsVar(1), xsArg(0), xsID_esp32);

		if (xsmcHas(xsVar(1), xsID_calibrated)) {
			xsmcGet(xsVar(2), xsVar(1), xsID_calibrated);
			calib = xsmcTest(xsVar(2));
		}

		if (xsmcHas(xsVar(1), xsID_attenuation)) {
			double atten;
			uint8_t attenIdx;
			xsmcGet(xsVar(2), xsVar(1), xsID_attenuation);
			atten = xsmcToNumber(xsVar(2));
			if (atten == 0)
				attenIdx = 0;
			else if (atten == 2.5)
				attenIdx = 1;
			else if (atten == 6)
				attenIdx = 2;
			else if (atten == 12)
				attenIdx = 3;
			else
				xsRangeError("bad attenuation");

			attenuation = &kAttenuationTable[attenIdx];
		}
	}

	for (i = 0; i < SOC_ADC_CHANNEL_NUM(0); i++) {
		if (gADCMap[i] == pin) {
			channel = i;
			port = 1;
			break;
		}
	}

#if ADC_PORTS > (1)
	if (channel < 0) {
		for (i = 0; i < SOC_ADC_CHANNEL_NUM(1); i++) {
			if (gADC2Map[i] == pin) {
				channel = i;
				port = 2;
				break;
			}
		}
	}
#endif

	if (channel < 0)
		xsRangeError("not analog pin");

	builtinInitializeTarget(the);
	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");


#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED || ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
	if (!calib)
		;
	else if (port == 1) {
		if (0 == gADC1_caliCount) {
			gADC1_cali_config.unit_id = ADC_UNIT_1;
			gADC1_cali_config.bitwidth = ADC_WIDTH;
			gADC1_cali_config.atten = attenuation->atten;

			#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
			err = adc_cali_create_scheme_curve_fitting(&gADC1_cali_config, &gADC1_cali_handle);
			#else
			err = adc_cali_create_scheme_line_fitting(&gADC1_cali_config, &gADC1_cali_handle);
			#endif

			if (err)
				modLog("analog calibration scheme failed P1");
		}
		gADC1_caliCount++;
	}
#if ADC_PORTS > (1)
	 else if (port == 2) {
		if (0 == gADC2_caliCount) {
			gADC2_cali_config.unit_id = ADC_UNIT_2;
			gADC2_cali_config.bitwidth = ADC_WIDTH;
			gADC2_cali_config.atten = attenuation->atten;

			#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
			err = adc_cali_create_scheme_curve_fitting(&gADC1_cali_config, &gADC1_cali_handle);
			#else
			err = adc_cali_create_scheme_line_fitting(&gADC2_cali_config, &gADC2_cali_handle);
			#endif

			if (err)
				modLog("analog calibration scheme failed P2");
		}
		gADC2_caliCount++;
	}
#endif

#endif

	analog = c_malloc(sizeof(AnalogRecord));
	if (!analog)
		xsRangeError("no memory");

	analog->obj = xsThis;
	xsRemember(analog->obj);
	xsmcSetHostData(xsThis, analog);
	analog->pin = pin;
	analog->port = port;
	analog->channel = channel;
	analog->calib = calib;
	analog->attenuation = attenuation;

    builtinUsePin(pin);

	adc_oneshot_unit_init_cfg_t unit_cfg = {0};
	adc_oneshot_chan_cfg_t config = {
		.bitwidth = ADC_WIDTH,
		.atten = attenuation->atten,
	};

	if (port == 1) {
		unit_cfg.unit_id = ADC_UNIT_1;
	} 
#if ADC_PORTS > (1)
	else if (port == 2) {
		unit_cfg.unit_id = ADC_UNIT_2;
	}
#endif
	unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;

	adc_oneshot_new_unit(&unit_cfg, &analog->handle);
	adc_oneshot_config_channel(analog->handle, channel, &config);
}

void xs_analog_destructor_(void *data)
{
	Analog analog = data;
	if (!analog)
		return;

	adc_oneshot_del_unit(analog->handle);

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED || ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
	if (!analog->calib)
		;
	else if (1 == analog->port) {
		if (0 == --gADC1_caliCount) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
			adc_cali_delete_scheme_curve_fitting(gADC1_cali_handle);
#else
			adc_cali_delete_scheme_line_fitting(gADC1_cali_handle);
#endif
		}
	}
#if ADC_PORTS > (1)
	else {
		if (0 == --gADC2_caliCount) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
			adc_cali_delete_scheme_curve_fitting(gADC2_cali_handle);
#else
			adc_cali_delete_scheme_line_fitting(gADC2_cali_handle);
#endif
		}
	}
#endif
#endif
    builtinFreePin(analog->pin);

	c_free(analog);
}

void xs_analog_close_(xsMachine *the)
{
	Analog analog = xsmcGetHostData(xsThis);
	if (analog && xsmcGetHostDataValidate(xsThis, xs_analog_destructor_)) {
		xsForget(analog->obj);
		xs_analog_destructor_(analog);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_analog_read_(xsMachine *the)
{
	int millivolts = -1;
	int raw;
	Analog analog = xsmcGetHostDataValidate(xsThis, xs_analog_destructor_);
	esp_err_t err;

	if (ESP_OK != adc_oneshot_read(analog->handle, analog->channel, &raw)) {
		modLog("analog onshot_read failed");
		return;
	}
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED || ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
	if (analog->calib) {
		if (analog->port == 1) {
			adc_cali_raw_to_voltage(gADC1_cali_handle, raw, &millivolts);
		} 
	#if ADC_PORTS > (1)
		else if (analog->port == 2) {
			adc_cali_raw_to_voltage(gADC2_cali_handle, raw, &millivolts);
		}
	#endif
		if (-1 == millivolts) {
			millivolts = (raw * analog->attenuation->Vmax) / ((1 << ADC_RESOLUTION) - 1);
			if (millivolts < analog->attenuation->Vmin)
				millivolts = analog->attenuation->Vmin;
			else if (millivolts > analog->attenuation->Vmax)
				millivolts = analog->attenuation->Vmax;
		}
	}
#endif

	if (analog->calib) {
		// convert calibrated analog back to ADC range
		raw = millivolts * ((1 << ADC_RESOLUTION)-1) / analog->attenuation->Vmax;	// 3300;
		if (raw > (1 << ADC_RESOLUTION) - 1)
			raw = (1 << ADC_RESOLUTION) - 1;
	}

	xsmcSetInteger(xsResult, raw);
}

void xs_analog_get_resolution_(xsMachine *the)
{
	xsmcSetInteger(xsResult, ADC_RESOLUTION);
}

