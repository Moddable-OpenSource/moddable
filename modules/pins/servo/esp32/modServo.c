/*
 * Copyright (c) 2016-2019 Moddable Tech, Inc.
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
#include "driver/ledc.h"

typedef struct {
	int			min;
	int			max;
	uint8_t		running;
	uint8_t		channel;
} modServoRecord, *modServo;

static uint8_t gChannelsUsed = 0;

void xs_servo_destructor(void *data)
{
	if (data) {
		modServo ms = (modServo)data;
		if (ms->running){
			ledc_stop(LEDC_HIGH_SPEED_MODE, ms->channel, 0);
			gChannelsUsed &= ~(1 << ms->channel);
		}
	}
}

void xs_servo(xsMachine *the)
{
	modServoRecord ms;
	int pin;
	double d;
	ledc_timer_config_t ledc_timer = {
		.bit_num = LEDC_TIMER_15_BIT,
		.freq_hz = 50,
		.speed_mode = LEDC_HIGH_SPEED_MODE,
		.timer_num = LEDC_TIMER_0
	};
	ledc_channel_config_t ledc_channel = {
		.channel    = LEDC_CHANNEL_0,
		.duty       = 0,
		.gpio_num   = 0,
		.speed_mode = LEDC_HIGH_SPEED_MODE,
		.timer_sel  = LEDC_TIMER_0
	};

	ms.min = 890;
	ms.max = 4000;
	ms.channel = LEDC_CHANNEL_0;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	ledc_channel.gpio_num = xsmcToInteger(xsVar(0));

	if (xsmcHas(xsArg(0), xsID_channel)){
		xsmcGet(xsVar(0), xsArg(0), xsID_channel);
		ledc_channel.channel = xsmcToInteger(xsVar(0));
		ms.channel = ledc_channel.channel;
	}else{
		uint8_t channels = gChannelsUsed;
		uint8_t channelNumber = 0;
		while ((channels & 1) && channelNumber < LEDC_CHANNEL_MAX){
			channelNumber++;
			channels >>= 1;
		}
		if (channelNumber == LEDC_CHANNEL_MAX){
			xsRangeError("all servo channels are already taken");
		}else{
			ledc_channel.channel = channelNumber;
			ms.channel = channelNumber;
			gChannelsUsed |= (1 << channelNumber);
		}
	}

	if (xsmcHas(xsArg(0), xsID_min)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_min);
		d = xsmcToNumber(xsVar(0));
		ms.min = (int)((d / 20000.0) * 32767.0);
	}

	if (xsmcHas(xsArg(0), xsID_max)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_max);
		d = xsmcToNumber(xsVar(0));
		ms.max = (int)((d / 20000.0) * 32767.0);
	}

	ledc_timer_config(&ledc_timer);
	ledc_channel_config(&ledc_channel);

	ms.running = true;
	xsmcSetHostChunk(xsThis, &ms, sizeof(modServoRecord));
}

void xs_servo_close(xsMachine *the)
{
	modServo ms = (modServo)xsmcGetHostChunk(xsThis);
	if (!ms || !ms->running) return;

	ledc_stop(LEDC_HIGH_SPEED_MODE, ms->channel, 0);
	ms->running = false;
	gChannelsUsed &= ~(1 << ms->channel);
}

void xs_servo_write(xsMachine *the)
{
	modServo ms = (modServo)xsmcGetHostChunk(xsThis);
	double degrees = xsmcToNumber(xsArg(0));
	int duty;

	if (!ms || !ms->running) xsUnknownError((char *)"closed");

	duty = (((double)(ms->max - ms->min) * degrees) / 180.0) + ms->min;

	ledc_set_duty(LEDC_HIGH_SPEED_MODE, ms->channel, duty);
	ledc_update_duty(LEDC_HIGH_SPEED_MODE, ms->channel);
}

void xs_servo_writeMicroseconds(xsMachine *the)
{
	modServo ms = (modServo)xsmcGetHostChunk(xsThis);
	double us = xsmcToNumber(xsArg(0));
	int duty;

	if (!ms || !ms->running) xsUnknownError((char *)"closed");

	duty = (int)((us / 20000.0) * 32767.0);

	ledc_set_duty(LEDC_HIGH_SPEED_MODE, ms->channel, duty);
	ledc_update_duty(LEDC_HIGH_SPEED_MODE, ms->channel);
}
