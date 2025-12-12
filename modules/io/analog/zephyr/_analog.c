/*
 * Copyright (c) 2019-2025  Moddable Tech, Inc.
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
	Analog -
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"			// esp platform support
#include "mc.xs.h"			// for xsID_* values
#include "mc.devicetree.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include "builtinCommon.h"

#if kModZephyrAnalogBusCount

struct AnalogRecord {
	xsSlot		obj;
	uint32_t	pin;
	uint8_t		channel;
	uint8_t		channel_idx;
//	struct adc_sequence sequence;
//	const struct device *port;
};
typedef struct AnalogRecord AnalogRecord;
typedef struct AnalogRecord *Analog;

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
		!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
	#warning "No suitable analog devicetree overlay specified"
	#define NO_CHANNELS 1

	static const struct adc_dt_spec adc_channels[0];
#else
	#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
		ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

	static const struct adc_dt_spec adc_channels[] = {
		DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
	};
#endif

static int adc_channel_idx(int channel) {
	int i;
#if ! NO_CHANNELS
	for (i = 0; i < (sizeof(adc_channels)/sizeof(struct adc_dt_spec)); i++) {
		if (adc_channels[i].channel_id == channel)
			return i;
	}
#endif
	return -1;
}

void xs_analog_constructor_(xsMachine *the)
{
	Analog analog;
	uint32_t channel;
	xsSlot tmp;
	int err;
	int idx;

	xsmcVars(2);

	if (!xsmcHas(xsArg(0), xsID_port))
		xsRangeError("port required");
	if (!xsmcHas(xsArg(0), xsID_channel))
		xsRangeError("channel required");

	xsmcGet(tmp, xsArg(0), xsID_port);
	const struct modZephyrAnalog *port = modZephyrGetAnalog(xsmcToString(tmp));
	if (NULL == port)
		xsRangeError("bad_port");

	xsmcGet(xsVar(0), xsArg(0), xsID_channel);
	channel = builtinGetPin(the, &xsVar(0));		// 

	idx = adc_channel_idx(channel);
	if (-1 == idx)
		xsRangeError("bad_channel");
	if (!adc_is_ready_dt(&adc_channels[idx]))
		xsRangeError("adc controller not ready");

	err = adc_channel_setup_dt(&adc_channels[idx]);
	if (err < 0)
		xsRangeError("could not set up adc channel");

	builtinInitializeTarget(the);
	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");

	analog = c_malloc(sizeof(AnalogRecord));
	if (!analog)
		xsRangeError("no memory");

	analog->obj = xsThis;
	xsRemember(analog->obj);
	xsmcSetHostData(xsThis, analog);
	analog->channel = channel;
	analog->channel_idx = idx;
}

void xs_analog_destructor_(void *data)
{
	Analog analog = data;
	if (!analog)
		return;

	c_free(analog);
}

void xs_analog_close_(xsMachine *the)
{
	Analog analog = xsmcGetHostData(xsThis);;
	if (analog && xsmcGetHostDataValidate(xsThis, xs_analog_destructor_)) {
		xsForget(analog->obj);
		xs_analog_destructor_(analog);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_analog_read_(xsMachine *the)
{
	Analog analog = xsmcGetHostDataValidate(xsThis, xs_analog_destructor_);
	int err;
	uint16_t value;
	struct adc_sequence sequence = {
		.buffer = &value,
		.buffer_size = sizeof(value),
	};

	(void)adc_sequence_init_dt(&adc_channels[analog->channel_idx], &sequence);
	err = adc_read_dt(&adc_channels[analog->channel_idx], &sequence);
	if (err < 0)
		xsUnknownError("Analog read error");

	xsmcSetInteger(xsResult, value);
}

void xs_analog_get_resolution_(xsMachine *the)
{
	Analog analog = xsmcGetHostDataValidate(xsThis, xs_analog_destructor_);

	xsmcSetInteger(xsResult, adc_channels[analog->channel_idx].resolution);
}

#else	// !kModZephyrAnalogBusCount

void xs_analog_constructor_(xsMachine *the)
{
	xsUnknownError("no Analog");
}

void xs_analog_destructor_(void *) {}
void xs_analog_close_(xsMachine *the) {}
void xs_analog_read_(xsMachine *the) {}
void xs_analog_get_resolution_(xsMachine *the) {}

#endif
