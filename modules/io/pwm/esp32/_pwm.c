/* * Copyright (c) 2019-2024  Moddable Tech, Inc.
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

// #if CONFIG_IDF_TARGET_ESP32
// #define kSpeedMode (LEDC_HIGH_SPEED_MODE)
// #else
#define kSpeedMode	(LEDC_LOW_SPEED_MODE)
// #endif

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
	int			duty;
	xsSlot		obj;
};
typedef struct PWMRecord PWMRecord;
typedef struct PWMRecord *PWM;

#define LOW_RES	8
#define HI_RES	14

#if (ESP32 == 1) || (ESP32 == 2) || (ESP32 == 5) || (ESP32 == 6)
	// esp32, esp32s2, esp32c6, esp32h2
	static uint8_t MIN_HZ[HI_RES - LOW_RES + 1] = { 65, 33, 17, 9, 5, 3, 2 };
#elif (ESP32 == 4)
	// esp32c3
	static uint8_t MIN_HZ[HI_RES - LOW_RES + 1] = { 71, 36, 18, 9, 5, 3, 2 };
#elif (ESP32 == 3)
	// esp32s3
	static uint8_t MIN_HZ[HI_RES - LOW_RES + 1] = { 68, 34, 17, 9, 5, 3, 2 };
#else
	#error esp32 variant needs PWM measurement
#endif

static int INVALID_HZ(int hz, int resolution) 
{
	if (hz < MIN_HZ[resolution - LOW_RES])
		return -1;
	return 0;
}

void xs_pwm_constructor_(xsMachine *the)
{
	PWM pwm = NULL;
	int pin;
	int hz = 1024;
	int resolution = 10;
	int8_t i, freeTimer = -1, timerIndex = -1;
	int ledc = -1, duty = 0;
	ledc_channel_config_t ledcConfig = {0};

	xsmcVars(2);

    if (xsmcHas(xsArg(0), xsID_from)) {
		xsmcGet(xsVar(1), xsArg(0), xsID_from);
		pwm = xsmcGetHostDataValidate(xsVar(1), xs_pwm_destructor_);
		pin = pwm->pin;
		ledc = pwm->ledc;
		duty = pwm->duty;
		hz = gTimers[pwm->timerIndex].hz;
		resolution = gTimers[pwm->timerIndex].resolution;
    }
    else {
		xsmcGet(xsVar(0), xsArg(0), xsID_pin);
		pin = builtinGetPin(the, &xsVar(0));

		if (!builtinIsPinFree(pin))
			xsUnknownError("in use");

		if (xsmcHas(xsArg(0), xsID_port)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_port);
			ledc = builtinGetSignedInteger(the, &xsVar(0));
			if ((ledc < 0) || (ledc >= LEDC_CHANNEL_MAX))
				xsRangeError("invalid port");
			if (!(gLEDC & (1 << ledc)))
				xsUnknownError("port unavailable");
		}
	}

    if (xsmcHas(xsArg(0), xsID_hz)) {
        xsmcGet(xsVar(0), xsArg(0), xsID_hz);
        hz = xsmcToInteger(xsVar(0));
        if (hz <= 0)
			xsRangeError("invalid hz");
    }

    if (xsmcHas(xsArg(0), xsID_resolution)) {
        xsmcGet(xsVar(0), xsArg(0), xsID_resolution);
        resolution = xsmcToInteger(xsVar(0));
        if ((resolution <= 0) || (resolution >= LEDC_TIMER_BIT_MAX))
			xsRangeError("invalid resolution");
    }

	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");

	if (INVALID_HZ(hz, resolution))
		xsRangeError("invalid combination of hz and resolution");

	if (-1 == ledc) {
		for (ledc = 0; ledc < 8; ledc++) {
			if (gLEDC & (1 << ledc))
				break;
		}
		if (8 == ledc)
			xsUnknownError("no ledc channel");
	}

	if (pwm && (1 == gTimers[pwm->timerIndex].useCount))
		freeTimer = pwm->timerIndex; // reuse, but reinitialize
	else {
		for (i = 0; i < LEDC_TIMER_MAX; i++) {
			if (!gTimers[i].useCount)
				freeTimer = i;
			else if ((resolution == gTimers[i].resolution) && (hz == gTimers[i].hz)) {
				timerIndex = i;
				break;
			}
		}
	}

	if (-1 == timerIndex) {
		ledc_timer_config_t t = {0};

		if (-1 == freeTimer)
			xsRangeError("no timer");

		timerIndex = freeTimer;

		t.speed_mode = kSpeedMode;
		t.duty_resolution = resolution;
		t.timer_num = timerIndex;
		t.freq_hz = hz;
		t.clk_cfg = LEDC_AUTO_CLK;

		ledc_timer_pause(kSpeedMode, timerIndex);

//		ledc_timer_rst(kSpeedMode, timerIndex);

		if (ESP_OK != ledc_timer_config(&t))
			xsUnknownError("configure ledc timer failed");

	}
	ledc_timer_resume(kSpeedMode, timerIndex);

	builtinInitializeTarget(the);

	ledcConfig.channel    = ledc;
	ledcConfig.duty       = duty;
	ledcConfig.gpio_num   = pin;
	ledcConfig.speed_mode = kSpeedMode;
	ledcConfig.timer_sel  = timerIndex;
	if (ESP_OK != ledc_channel_config(&ledcConfig))
		xsUnknownError("can't configure");

	if (NULL == pwm) {
		pwm = c_malloc(sizeof(PWMRecord));
		if (!pwm) {
			//@@ disable ledc_channel
			xsRangeError("no memory");
		}
	}
	else {
		gTimers[pwm->timerIndex].useCount -= 1;
		xsForget(pwm->obj);
		xsmcSetHostData(xsVar(1), NULL);
		xsmcSetHostDestructor(xsVar(1), NULL);
	}

	pwm->pin = pin;
	pwm->timerIndex = timerIndex;
	pwm->ledc = ledc;
	pwm->duty = duty;
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
	if (0 == gTimers[pwm->timerIndex].useCount) {
		gpio_set_direction(pwm->pin, GPIO_MODE_OUTPUT);		// this seems to disconnect pin from ledc / timer
		gTimers[pwm->timerIndex].resolution = 0;
		gTimers[pwm->timerIndex].hz = 0;

		esp_err_t err;
		err =		ledc_timer_pause(kSpeedMode, pwm->timerIndex);
		if (err)
			modLog("ledc_timer_pause failed");

		ledc_timer_config_t t = {0};

		t.speed_mode = kSpeedMode;
		t.timer_num = pwm->timerIndex;
		t.duty_resolution = gTimers[pwm->timerIndex].resolution;
		t.freq_hz = gTimers[pwm->timerIndex].hz;
		t.clk_cfg = LEDC_AUTO_CLK;
		t.deconfigure = true;		// introduced in 5.2.1

		err = ledc_timer_config(&t);
		if (err)
			modLog("ledc_timer_config failed");
	}

    builtinFreePin(pwm->pin);

    c_free(pwm);
}

void xs_pwm_close_(xsMachine *the)
{
	PWM pwm = xsmcGetHostData(xsThis);
	if (pwm && xsmcGetHostDataValidate(xsThis, xs_pwm_destructor_)) {
		xsForget(pwm->obj);
		xs_pwm_destructor_(pwm);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_pwm_write_(xsMachine *the)
{
	PWM pwm = xsmcGetHostDataValidate(xsThis, xs_pwm_destructor_);
	int max = (1 << gTimers[pwm->timerIndex].resolution) - 1;
	int value = xsmcToInteger(xsArg(0));

	if ((value < 0) || (value > max))
		xsRangeError("invalid value");
	if (value == max)
		value += 1;
	ledc_set_duty(kSpeedMode, pwm->ledc, value);
	ledc_update_duty(kSpeedMode, pwm->ledc);
	pwm->duty = value;
}

void xs_pwm_get_hz_(xsMachine *the)
{
	PWM pwm = xsmcGetHostDataValidate(xsThis, xs_pwm_destructor_);

	xsmcSetInteger(xsResult, gTimers[pwm->timerIndex].hz);
}

void xs_pwm_get_resolution_(xsMachine *the)
{
	PWM pwm = xsmcGetHostDataValidate(xsThis, xs_pwm_destructor_);

	xsmcSetInteger(xsResult, gTimers[pwm->timerIndex].resolution);
}
