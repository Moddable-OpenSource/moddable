/*
 * Copyright (c) 2018-2019  Moddable Tech, Inc.
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

#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#include "driver/ledc.h"
#include "stdlib.h"

// default is channels 4 through 7
#ifndef MODDEF_PWM_LEDC_CHANNEL
	#define MODDEF_PWM_LEDC_CHANNEL_MAP 0xF0
#endif
#ifndef MODDEF_PWM_LEDC_TIMER
	#define MODDEF_PWM_LEDC_TIMER LEDC_TIMER_0
#endif

#if IDF_TARGET == esp32s2
	#define ESP_SPEED_MODE LEDC_LOW_SPEED_MODE
#else
	#define ESP_SPEED_MODE LEDC_HIGH_SPEED_MODE
#endif

static const ledc_timer_config_t gTimer = {
	.duty_resolution = LEDC_TIMER_10_BIT,
	.freq_hz = 1024,
//	.speed_mode = LEDC_HIGH_SPEED_MODE,		// not for esp32s2
	.speed_mode = ESP_SPEED_MODE,
	.timer_num = MODDEF_PWM_LEDC_TIMER
};

static uint8_t gLEDC = MODDEF_PWM_LEDC_CHANNEL_MAP;

struct PWMRecord {
	uint32_t		gpio;
	uint8_t			ledc;
};
typedef struct PWMRecord PWMRecord;
typedef struct PWMRecord *PWM;

void xs_pwm_destructor(void *data)
{
	PWM pwm = data;
	if (pwm) return;

	ledc_set_duty(ESP_SPEED_MODE, pwm->ledc, pwm->gpio);
	ledc_update_duty(ESP_SPEED_MODE, pwm->ledc);

	gLEDC |= 1 << pwm->ledc;

	free(pwm);
}

void xs_pwm(xsMachine *the)
{
	static uint8_t initialized = false;
	int gpio;
	ledc_channel_config_t ledcConfig;
	PWM pwm;
	int ledc;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	gpio = xsmcToInteger(xsVar(0));

	if (xsmcHas(xsArg(0), xsID_port))
		xsUnknownError("no port - esp32");

	if (!initialized) {
		if (ESP_OK != ledc_timer_config(&gTimer))
			xsUnknownError("configure ledc timer failed");
		initialized = true;
	}

	for (ledc = 0; ledc < 8; ledc++) {
		if (gLEDC & (1 << ledc))
			break;
	}
	if (8 == ledc)
		xsUnknownError("no ledc channel free");

	ledcConfig.channel    = ledc;
	ledcConfig.duty       = 0;
	ledcConfig.gpio_num   = gpio;
	ledcConfig.speed_mode = ESP_SPEED_MODE;
	ledcConfig.timer_sel  = MODDEF_PWM_LEDC_TIMER;

	if (ESP_OK != ledc_channel_config(&ledcConfig))
		xsUnknownError("configure ledc channel failed");

	pwm = (PWM)malloc(sizeof(PWMRecord));
	if (!pwm)
		xsUnknownError("no memory");
	pwm->gpio = gpio;
	pwm->ledc = ledc;

	xsmcSetHostData(xsThis, pwm);

	gLEDC &= ~(1 << ledc);
}

void xs_pwm_close(xsMachine *the)
{
	PWM pwm = xsmcGetHostData(xsThis);
	if (!pwm) return;
	xs_pwm_destructor(pwm);
	xsmcSetHostData(xsThis, NULL);
}

void xs_pwm_write(xsMachine *the)
{
	PWM pwm = xsmcGetHostData(xsThis);
	int value = xsmcToInteger(xsArg(0));	// 0 to 1023

	if (!pwm) return;

	if ((value < 0) || (value >= gTimer.freq_hz))
		xsRangeError("bad value");

	ledc_set_duty(ESP_SPEED_MODE, pwm->ledc, value);
	ledc_update_duty(ESP_SPEED_MODE, pwm->ledc);
}
