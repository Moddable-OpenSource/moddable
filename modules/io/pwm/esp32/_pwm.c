/*
 * Copyright (c) 2019-2021  Moddable Tech, Inc.
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
	PWM - uing ESP-IDF
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"			// esp platform support
#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

#include "driver/ledc.h"

// default is channels 4 through 7
#ifndef MODDEF_PWM_LEDC_CHANNEL
	#define MODDEF_PWM_LEDC_CHANNEL_MAP 0xF0
#endif

#define kSpeedMode (LEDC_LOW_SPEED_MODE)

struct LEDCTimerRecord {
	uint16_t	useCount;
	uint16_t	resolution;
	int			hz;
};
typedef struct LEDCTimerRecord LEDCTimerRecord;
typedef struct LEDCTimerRecord *LEDCTimer;

static LEDCTimerRecord gTimers[LEDC_TIMER_MAX];

static uint8_t gLEDC = MODDEF_PWM_LEDC_CHANNEL_MAP;

struct PWMRecord {
	uint16_t	pin;
	uint8_t		timerIndex;
	uint8_t		ledc;
	xsSlot		obj;
};
typedef struct PWMRecord PWMRecord;
typedef struct PWMRecord *PWM;

void xs_pwm_constructor_(xsMachine *the)
{
	PWM pwm;
	int pin;
	int hz = 1024;
	int resolution = 10;
	int8_t i, free = -1, timerIndex = -1;
	int ledc = -1;
	ledc_channel_config_t ledcConfig;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = xsmcToInteger(xsVar(0));

    if (!builtinIsPinFree(pin))
		xsRangeError("in use");

    if (xsmcHas(xsArg(0), xsID_hz)) {
        xsmcGet(xsVar(0), xsArg(0), xsID_hz);
        hz = xsmcToInteger(xsVar(0));
        if (hz < 0)
			xsRangeError("invalid hz");
    }

    if (xsmcHas(xsArg(0), xsID_resolution)) {
        xsmcGet(xsVar(0), xsArg(0), xsID_resolution);
        resolution = xsmcToInteger(xsVar(0));
        if ((resolution < 0) || (resolution >= LEDC_TIMER_BIT_MAX))
			xsRangeError("invalid resolution");
    }

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		ledc = xsmcToInteger(xsVar(0));
		if ((ledc < 0) || (ledc >= LEDC_CHANNEL_MAX))
			xsRangeError("invalid port");
		if (!(gLEDC & (1 << ledc)))
			xsRangeError("port unavailable");
	}

	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");

	if (-1 == ledc) {
		for (ledc = 0; ledc < 8; ledc++) {
			if (gLEDC & (1 << ledc))
				break;
		}
		if (8 == ledc)
			xsUnknownError("no ledc channel");
	}

	for (i = 0; i < LEDC_TIMER_MAX; i++) {
		if (!gTimers[i].useCount)
			free = i;
		else if ((resolution == gTimers[i].resolution) && (hz == gTimers[i].hz)) {
			timerIndex = i;
			break;
		}
	}

	if (-1 == timerIndex) {
		ledc_timer_config_t t;

		if (-1 == free)
			xsRangeError("no timer");

		timerIndex = free;

		t.speed_mode = kSpeedMode;
		t.duty_resolution = resolution;
		t.timer_num = timerIndex;
		t.freq_hz = hz;
		t.clk_cfg = LEDC_AUTO_CLK;

		if (ESP_OK != ledc_timer_config(&t))
			xsUnknownError("configure ledc timer failed");
	}

	builtinInitializeTarget(the);

	ledcConfig.channel    = ledc;
	ledcConfig.duty       = 0;
	ledcConfig.gpio_num   = pin;
	ledcConfig.speed_mode = kSpeedMode;
	ledcConfig.timer_sel  = timerIndex;
	if (ESP_OK != ledc_channel_config(&ledcConfig))
		xsUnknownError("can't configure");

	pwm = c_malloc(sizeof(PWMRecord));
	if (!pwm) {
		//@@ disable ledc_channel
		xsRangeError("no memory");
	}

	pwm->pin = pin;
	pwm->timerIndex = timerIndex;
	pwm->ledc = ledc;
	pwm->obj = xsThis;

	xsRemember(pwm->obj);
    xsmcSetHostData(xsThis, pwm);

	gTimers[timerIndex].useCount += 1;
	gTimers[timerIndex].resolution = resolution;
	gTimers[timerIndex].hz = hz;

	gLEDC &= ~(1 << ledc);
    builtinUsePin(pin);
}

void xs_pwm_destructor_(void *data)
{
	PWM pwm = data;
	if (!pwm) return;

	ledc_stop(kSpeedMode, pwm->ledc, 0);
	gLEDC |= 1 << pwm->ledc;

	gTimers[pwm->timerIndex].useCount -= 1;
//	if (0 == gTimers[pwm->timerIndex].useCount)
//		;		/@@ how to stop the timer?

    builtinFreePin(pwm->pin);

    c_free(pwm);
}

void xs_pwm_close_(xsMachine *the)
{
	PWM pwm = xsmcGetHostData(xsThis);
	if (!pwm) return;

	xsForget(pwm->obj);
	xs_pwm_destructor_(pwm);
	xsmcSetHostData(xsThis, NULL);
}

void xs_pwm_write_(xsMachine *the)
{
    int value, max;
	PWM pwm = xsmcGetHostData(xsThis);
	if (!pwm)
		xsUnknownError("closed");

	max = (1 << gTimers[pwm->timerIndex].resolution) - 1;
    value = xsmcToInteger(xsArg(0));
	if ((value < 0) || (value > max))
		xsUnknownError("invalid value");
	if (value == max)
		value += 1;
	ledc_set_duty(kSpeedMode, pwm->ledc, value);
	ledc_update_duty(kSpeedMode, pwm->ledc);
}

void xs_pwm_get_hz_(xsMachine *the)
{
	PWM pwm = xsmcGetHostData(xsThis);
	if (!pwm)
		return;

	xsmcSetInteger(xsResult, gTimers[pwm->timerIndex].hz);
}

void xs_pwm_get_resolution_(xsMachine *the)
{
	PWM pwm = xsmcGetHostData(xsThis);
	if (!pwm)
		return;

	xsmcSetInteger(xsResult, gTimers[pwm->timerIndex].resolution);
}
