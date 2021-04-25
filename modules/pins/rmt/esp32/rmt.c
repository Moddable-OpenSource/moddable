/*
 * Copyright (c) 2018-2021 Moddable Tech, Inc.
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

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values

#include "driver/rmt.h"

typedef struct modRMTRecord modRMTRecord;
typedef modRMTRecord *modRMT;

struct modRMTRecord {
	modRMT			next;

	xsMachine		*the;
	xsSlot			obj;
	int				channel;
	void			*transmitting;
	RingbufHandle_t	rb;
};

static void rmtTransmitComplete(rmt_channel_t channel, void *arg);
static void rmtWritable(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static int receive(modRMT rmt, uint8_t *buffer, int maxBytes, int* count, int* phase);
static int getInt(xsMachine *the, xsSlot *options, xsIdentifier id);

static modRMT gRMT;

void xs_rmt_destructor(void *data)
{
	if (data) {
		modRMT rmt = data;
		modRMT walker;

		if (rmt->transmitting)
			c_free(rmt->transmitting);

		//@@ critical section
		if (gRMT == rmt)
			gRMT = rmt->next;
		else {
			for (walker = gRMT; NULL != walker; walker = walker->next) {
				if (walker->next == rmt) {
					walker->next = rmt->next;
					break;
				}
			}
		}

		rmt_driver_uninstall(rmt->channel);

		c_free(data);

		if (NULL == gRMT)
			rmt_register_tx_end_callback(NULL, NULL);
	}
}

void xs_rmt(xsMachine *the)
{
	modRMT rmt;
	int pin = 0, channel = RMT_CHANNEL_0, divider = 255, timeout = 5000, filter = 0, transmit = 1, ringbuffer = 1024;
	esp_err_t err;
	rmt_config_t config = {0};

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = xsmcToInteger(xsVar(0));
	
	if (xsmcHas(xsArg(0), xsID_channel)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_channel);
		channel = xsmcToInteger(xsVar(0));
	}
	if (xsmcHas(xsArg(0), xsID_divider)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_divider);
		divider = xsmcToInteger(xsVar(0));
	}
	if (xsmcHas(xsArg(0), xsID_direction)){
		char* direction;
		xsmcGet(xsVar(0), xsArg(0), xsID_direction);
		direction = xsmcToString(xsVar(0));
		if (c_strcmp(direction, "rx") == 0) transmit = 0;
	}
	if (xsmcHas(xsArg(0), xsID_filter)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_filter);
		filter = xsmcToInteger(xsVar(0));
	}
	if (xsmcHas(xsArg(0), xsID_timeout)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_timeout);
		timeout = xsmcToInteger(xsVar(0));
	}
	if (xsmcHas(xsArg(0), xsID_ringbufferSize)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_ringbufferSize);
		ringbuffer = xsmcToInteger(xsVar(0));
	}
	if (xsmcHas(xsArg(0), xsID_tx_config)) {
		xsSlot options;
		xsmcGet(options, xsArg(0), xsID_tx_config);
		config.tx_config.loop_en = getInt(the, &options, xsID_loop_en);
		config.tx_config.carrier_freq_hz = getInt(the, &options, xsID_carrier_freq_hz);
		config.tx_config.carrier_duty_percent = getInt(the, &options, xsID_carrier_duty_percent);
		config.tx_config.carrier_level = getInt(the, &options, xsID_carrier_level);
		config.tx_config.carrier_en = getInt(the, &options, xsID_carrier_en);
		config.tx_config.idle_level = getInt(the, &options, xsID_idle_level);
		config.tx_config.idle_output_en = getInt(the, &options, xsID_idle_output_en);
	}
	else {
		config.tx_config.idle_output_en = 1;
	}

	rmt = c_calloc(1, sizeof(modRMTRecord));
	if (!rmt)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, rmt);

	rmt->channel = channel;
	rmt->the = the;
	rmt->obj = xsThis;

	config.rmt_mode = transmit ? RMT_MODE_TX : RMT_MODE_RX;
    config.channel = channel;
    config.gpio_num = pin;
    config.mem_block_num = 1;
	config.clk_div = divider;

	if (!transmit) {
		if (filter) {
			config.rx_config.filter_en = true;
			config.rx_config.filter_ticks_thresh = filter;
		}
		else {
			config.rx_config.filter_en = false;
		}
		config.rx_config.idle_threshold = timeout;
	}
	
	err = rmt_config(&config);
	if (ESP_OK != err)
		xsUnknownError("config failed");

	err = rmt_driver_install(channel, transmit ? 0 : ringbuffer, 0);
	if (ESP_OK != err)
		xsUnknownError("install failed");
		
	if (transmit){
		if (NULL == gRMT) rmt_register_tx_end_callback(rmtTransmitComplete, NULL);
			//@@ critical section
			rmt->next = gRMT;
		gRMT = rmt;
		modMessagePostToMachine(the, NULL, 0, rmtWritable, rmt); // initial onWritable
	}else{
		rmt_get_ringbuf_handle(channel, &rmt->rb);
		rmt_rx_start(channel, 1);
	}
	
}

void xs_rmt_close(xsMachine *the)
{
	modRMT rmt = xsmcGetHostData(xsThis);
	xs_rmt_destructor(rmt);
	xsmcSetHostData(xsThis, NULL);
}

void xs_rmt_read(xsMachine *the){
	uint8_t *data;
	int err, count, phase;
	xsmcVars(1);

	modRMT rmt = xsmcGetHostData(xsThis);
  
  	int bufferByteLength = xsmcGetArrayBufferLength(xsArg(0));
  	data = (uint8_t*)xsmcToArrayBuffer(xsArg(0));

	err = receive(rmt, data, bufferByteLength, &count, &phase);
	if (err == -1) xsRangeError("ringbuffer has too much data");
	xsResult = xsmcNewObject();
	xsmcSetInteger(xsVar(0), phase);
	xsmcSet(xsResult, xsID_phase, xsVar(0));
	xsmcSetInteger(xsVar(0), count);
	xsmcSet(xsResult, xsID_count, xsVar(0));
	xsmcSet(xsResult, xsID_buffer, xsArg(0));
}

void xs_rmt_write(xsMachine *the)
{
	modRMT rmt = xsmcGetHostData(xsThis);
	esp_err_t err;
	int count, length, i;
	int value, duration;
	rmt_item32_t *items, *item;
	uint8_t phase = 0;

	if (rmt->transmitting)
		xsUnknownError("write in progress");

	value = xsmcToInteger(xsArg(0));

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(1), xsID_length);
	length = xsmcToInteger(xsVar(0));
	count = length + 2;	// (at least) one entry for each array item plus 2 terminating entries
	for (i = 0, item = items; i < length; i++) {
		xsmcGetIndex(xsVar(0), xsArg(1), i);
		duration = xsmcToInteger(xsVar(0));
		if (duration > 32767)
			count += duration / 32767;
	}

	items = c_malloc(((count + 1) >> 1) * sizeof(rmt_item32_t));
	if (!items)
		xsUnknownError("no memory");

	for (i = 0, duration = 0, item = items; (i < length) || duration;) {
		int use;

		if (0 == duration) {
			value ^= 1;
			xsmcGetIndex(xsVar(0), xsArg(1), i);
			duration = xsmcToInteger(xsVar(0));
			i++;
		}
		use = duration;
		if (duration > 32767)
			use = 32767;

		if (!(phase & 1)) {
			item->duration0 = use;
			item->level0 = value ? 0 : 1;
		}
		else {
			item->duration1 = use;
			item->level1 = value ? 0 : 1;
			item += 1;
		}
		phase ^= 1;

		duration -= use;
	}
	if (phase) {
		item->duration1 = 0;
		item->level1 = 0;
		item += 1;
	}
	item->duration0 = 0;
	item->level0 = 1;
	item->duration1 = 0;
	item->level1 = 0;
	item += 1;

	err = rmt_write_items(rmt->channel, items, item - items, false);
	if (ESP_OK != err) {
		c_free(items);
		xsUnknownError("write failed");
	}

	rmt->transmitting = items;
}

void rmtWritable(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modRMT rmt = refcon;

	if (rmt->transmitting) {
		c_free(rmt->transmitting);
		rmt->transmitting = NULL;
	}

	xsBeginHost(the);
		xsCall0(rmt->obj, xsID_onWritable);
	xsEndHost(the);
}

void rmtTransmitComplete(rmt_channel_t channel, void *arg)
{
	modRMT rmt;

	for (rmt = gRMT; NULL != rmt; rmt = rmt->next) {
		if (rmt->channel == channel)
			break;
	}
	if (!rmt)
		return;		// corrupt

	modMessagePostToMachineFromISR(rmt->the, rmtWritable, rmt);
}

int receive(modRMT rmt, uint8_t *buffer, int maxBytes, int* count, int* phase){
	size_t rx_size = 0;
	uint16_t *on = (uint16_t*) buffer;
	int dummy;
	
	rmt_item32_t *item = (rmt_item32_t *)xRingbufferReceiveFromISR(rmt->rb, &rx_size);
	
	if (rx_size > maxBytes) return -1;

	*count = (rx_size >> 1);

	if (item){
		rmt_item32_t *first_item = item;
		*phase = item->level0;

		while (rx_size > 0){
			*on++ = item->duration0;
			*on++ = item->duration1;
			item++;
			rx_size -= 4;
		}
		vRingbufferReturnItemFromISR(rmt->rb, (void*) first_item, &dummy);
	}
	
	return 0;
}

int getInt(xsMachine *the, xsSlot *options, xsIdentifier id)
{
	if (xsmcHas(*options, id)) {
		xsmcGet(xsVar(0), *options, id);
		return xsmcToInteger(xsVar(0));
	}

	return 0;
}
