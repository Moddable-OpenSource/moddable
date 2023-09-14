/*
 * Copyright (c) 2023 Moddable Tech, Inc.
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

#include "boschlib/bme68x.h"
#include "boschlib/bme68x_defs.h"

#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"			// for xsID_ values

struct BME68xlib {
	xsMachine 				*the;
	xsSlot					obj;
	struct bme68x_dev		bd;
};
typedef struct BME68xlib BME68xlib;

static BME68X_INTF_RET_TYPE libRead(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
	BME68xlib *lib = intf_ptr;
	xsMachine *the = lib->the;

	xsmcSetInteger(xsVar(0), reg_addr);
	xsmcSetInteger(xsVar(1), length);
	xsResult = xsCall2(lib->obj, xsID_read, xsVar(0), xsVar(1));

	void *data;
	xsUnsignedValue count;
	xsmcGetBufferReadable(xsResult, &data, &count);
	if (count != length)
		xsUnknownError("bad length");
	c_memmove(reg_data, data, count);
	return BME68X_OK;
}

static BME68X_INTF_RET_TYPE libWrite(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
	BME68xlib *lib = intf_ptr;
	xsMachine *the = lib->the;
	uint8_t buffer[32];

	if ((length + 1) > sizeof(buffer))
		xsUnknownError("write too long");

	buffer[0] = reg_addr;
	c_memmove(buffer + 1, reg_data, length);

	xsmcSetArrayBuffer(xsVar(0), buffer, length + 1);
	xsCall1(lib->obj, xsID_write, xsVar(0));
	return BME68X_OK;
}

static void libDelay(uint32_t period, void *intf_ptr)
{
	modDelayMicroseconds(period);
}

void xs_bne68x_destructor(void *data)
{
	if (!data) return;

	c_free(data);
}

void xs_bme68x_init(xsMachine *the)
{
	BME68xlib *lib;

	xsmcVars(2);

	lib = c_calloc(1, sizeof(BME68xlib));
	if (!lib)
		xsUnknownError("no memory");

	lib->obj = xsThis;
	lib->the = the;
	lib->bd.amb_temp = 25;		// all examples hardcode this. perhaps provide option to specify
	lib->bd.read = libRead;
	lib->bd.write = libWrite;
	lib->bd.delay_us = libDelay;
	lib->bd.intf = BME68X_I2C_INTF;
	lib->bd.intf_ptr = lib;
	
	xsmcSetHostData(xsThis, lib);

	if (0 != bme68x_init(&lib->bd))
		xsUnknownError("init fail");
}

static uint8_t getUint8(BME68xlib *lib, xsSlot *options, xsIdentifier property)
{
	xsMachine *the = lib->the;
	xsSlot tmp;

	xsmcGet(tmp, *options, property);
	if (xsUndefinedType == xsmcTypeOf(tmp))
		xsUnknownError("missing property");
	
	int result = xsmcToInteger(tmp);
	if ((result < 0) || (result > 255))
		xsRangeError("bad value");
		
	return (uint8_t)result;
}

static uint16_t getUint16(BME68xlib *lib, xsSlot *options, xsIdentifier property)
{
	xsMachine *the = lib->the;
	xsSlot tmp;

	xsmcGet(tmp, *options, property);
	if (xsUndefinedType == xsmcTypeOf(tmp))
		xsUnknownError("missing property");
	
	int result = xsmcToInteger(tmp);
	if ((result < 0) || (result > 65535))
		xsRangeError("bad value");
		
	return (uint16_t)result;
}

void xs_bme68x_set_conf(xsMachine *the)
{
	BME68xlib *lib = xsmcGetHostData(xsThis);
    struct bme68x_conf conf;

	xsmcVars(2);

	conf.os_hum = getUint8(lib, &xsArg(0), xsID_os_hum);
	conf.os_temp = getUint8(lib, &xsArg(0), xsID_os_temp);
	conf.os_pres = getUint8(lib, &xsArg(0), xsID_os_pres);
	conf.filter = getUint8(lib, &xsArg(0), xsID_filter);
	conf.odr = getUint8(lib, &xsArg(0), xsID_odr);

	if (0 != bme68x_set_conf(&conf, &lib->bd))
		xsUnknownError("set_conf fail");
}

void xs_bme68x_get_conf(xsMachine *the)
{
	BME68xlib *lib = xsmcGetHostData(xsThis);
    struct bme68x_conf conf;

	xsmcVars(2);

	if (0 != bme68x_get_conf(&conf, &lib->bd))
		xsUnknownError("get_conf fail");

	xsmcSetNewObject(xsResult);

	xsSlot tmp;
	xsmcSetInteger(tmp, conf.os_hum);
	xsmcSet(xsResult, xsID_os_hum, tmp);

	xsmcSetInteger(tmp, conf.os_temp);
	xsmcSet(xsResult, xsID_os_temp, tmp);

	xsmcSetInteger(tmp, conf.os_pres);
	xsmcSet(xsResult, xsID_os_pres, tmp);

	xsmcSetInteger(tmp, conf.filter);
	xsmcSet(xsResult, xsID_filter, tmp);

	xsmcSetInteger(tmp, conf.odr);
	xsmcSet(xsResult, xsID_odr, tmp);
}

void xs_bme68x_set_heatr_conf(xsMachine *the)
{
	BME68xlib *lib = xsmcGetHostData(xsThis);
    struct bme68x_heatr_conf conf;
    uint16_t heatr_temp_prof[10];
    uint16_t heatr_dur_prof[10];

	xsmcVars(2);

	int mode = xsmcToInteger(xsArg(0));
	if ((mode < 0) || (mode > 255))
		xsRangeError("bad value");

	conf.enable = getUint8(lib, &xsArg(1), xsID_enable);

	xsSlot array, lengthSlot;
	int lengthTmp = 0, lengthDur = 0, i;

	xsmcGet(array, xsArg(1), xsID_heatr_temp_prof);
	if (xsmcTest(array)) {
		conf.heatr_temp = 0;
	 	conf.heatr_temp_prof = heatr_temp_prof;
		xsmcGet(lengthSlot, array, xsID_length);
		lengthTmp = xsmcToInteger(lengthSlot);
		if ((lengthTmp < 0) || (lengthTmp > 10))
			xsRangeError("bad temp_prof");
		for (i = 0; i < lengthTmp; i++) {
			xsmcGetIndex(lengthSlot, array, i);
			heatr_temp_prof[i] = xsmcToInteger(lengthSlot);
		} 
	}
	else {
	 	conf.heatr_temp = getUint16(lib, &xsArg(1), xsID_heatr_temp);
	 	conf.heatr_temp_prof = NULL;
	} 

	xsmcGet(array, xsArg(1), xsID_heatr_dur_prof);
	if (xsmcTest(array)) {
		conf.heatr_dur = 0;
	 	conf.heatr_dur_prof = heatr_dur_prof;
		xsmcGet(lengthSlot, array, xsID_length);
		lengthDur = xsmcToInteger(lengthSlot);
		if ((lengthDur < 0) || (lengthDur > 10))
			xsRangeError("bad dur_prof");
		for (i = 0; i < lengthDur; i++) {
			xsmcGetIndex(lengthSlot, array, i);
			heatr_dur_prof[i] = xsmcToInteger(lengthSlot);
		} 
	}
	else {
		conf.heatr_dur = getUint16(lib, &xsArg(1), xsID_heatr_dur);
	 	conf.heatr_dur_prof = NULL;
	}

	if (lengthDur != lengthTmp)
		xsUnknownError("prof mismatch");
	conf.profile_len = lengthTmp;

	if (xsmcHas(xsArg(1), xsID_shared_heatr_dur))
		conf.shared_heatr_dur = getUint16(lib, &xsArg(1), xsID_shared_heatr_dur);
	else
		conf.shared_heatr_dur = 0;

	if (0 != bme68x_set_heatr_conf((uint8_t)mode, &conf, &lib->bd))
		xsUnknownError("set_heatr_conf fail");
}

// note: the implementation of bme68x_get_heatr_conf is incomplete, so this is currently useless 
void xs_bme68x_get_heatr_conf(xsMachine *the)
{
	BME68xlib *lib = xsmcGetHostData(xsThis);
    struct bme68x_heatr_conf conf = {0};
    uint8_t unused[10];

	conf.heatr_dur_prof = (void *)unused;
	conf.heatr_temp_prof = (void *)unused;
	conf.profile_len = sizeof(unused);	//@@
	if (0 != bme68x_get_heatr_conf(&conf, &lib->bd))
		xsUnknownError("get_heatr_conf fail");

	xsmcSetNewObject(xsResult);

	xsSlot tmp;
	xsmcSetInteger(tmp, conf.enable);
	xsmcSet(xsResult, xsID_enable, tmp);

	xsmcSetInteger(tmp, conf.heatr_temp);
	xsmcSet(xsResult, xsID_heatr_temp, tmp);

	xsmcSetInteger(tmp, conf.heatr_dur);
	xsmcSet(xsResult, xsID_heatr_dur, tmp);

	xsmcSetInteger(tmp, conf.shared_heatr_dur);
	xsmcSet(xsResult, xsID_shared_heatr_dur, tmp);
	
//@@ report conf.heatr_dur_prof conf.heatr_temp_prof

}

void xs_bme68x_set_op_mode(xsMachine *the)
{
	BME68xlib *lib = xsmcGetHostData(xsThis);
	int mode = xsmcToInteger(xsArg(0));
	if ((mode < 0) || (mode > 255))
		xsRangeError("bad value");

	xsmcVars(2);

	if (0 != bme68x_set_op_mode((uint8_t)mode, &lib->bd))
		xsUnknownError("set_op_mode fail");
}

void xs_bme68x_get_op_mode(xsMachine *the)
{
	BME68xlib *lib = xsmcGetHostData(xsThis);
	uint8_t op_mode;

	xsmcVars(2);

	if (0 != bme68x_get_op_mode(&op_mode, &lib->bd))
		xsUnknownError("get_op_mode fail");

	xsmcSetInteger(xsResult, op_mode);
}

void xs_bme68x_get_meas_dur(xsMachine *the)
{
	BME68xlib *lib = xsmcGetHostData(xsThis);
	int mode = xsmcToInteger(xsArg(0));
	if ((mode < 0) || (mode > 255))
		xsRangeError("bad value");
	struct bme68x_conf conf;

	xsmcVars(2);

	conf.os_hum = getUint8(lib, &xsArg(1), xsID_os_hum);
	conf.os_temp = getUint8(lib, &xsArg(1), xsID_os_temp);
	conf.os_pres = getUint8(lib, &xsArg(1), xsID_os_pres);
	conf.filter = getUint8(lib, &xsArg(1), xsID_filter);
	conf.odr = getUint8(lib, &xsArg(1), xsID_odr);

	uint32_t result = bme68x_get_meas_dur((uint8_t)mode, &conf, &lib->bd);
	xsmcSetNumber(xsResult, ((double)result) / 1000.0);		// millliseconds
}

void xs_bme68x_get_data(xsMachine *the)
{
	BME68xlib *lib = xsmcGetHostData(xsThis);
	int mode = xsmcToInteger(xsArg(0)), i;
	if ((mode < 0) || (mode > 255))
		xsRangeError("bad value");
	struct bme68x_data data[3];
    uint8_t n_fields;

	xsmcVars(2);

	int8_t rslt = bme68x_get_data((uint8_t)mode, data, &n_fields, &lib->bd);
	if (BME68X_W_NO_NEW_DATA == rslt)
		return;

	if (BME68X_OK != rslt)
		xsRangeError("get_data fail");

	xsmcSetNewArray(xsResult, 0);
	xsmcVars(1);
	for (i = 0; i < n_fields; i++) {
		struct bme68x_data *d = &data[i];
		xsmcSetNewObject(xsVar(0));
		xsmcSetIndex(xsResult, i, xsVar(0));

		xsSlot tmp;
		xsmcSetInteger(tmp, d->status);
		xsmcSet(xsVar(0), xsID_status, tmp);

		xsmcSetInteger(tmp, d->gas_index);
		xsmcSet(xsVar(0), xsID_gas_index, tmp);

		xsmcSetInteger(tmp, d->meas_index);
		xsmcSet(xsVar(0), xsID_meas_index, tmp);

		xsmcSetInteger(tmp, d->res_heat);
		xsmcSet(xsVar(0), xsID_res_heat, tmp);

		xsmcSetInteger(tmp, d->idac);
		xsmcSet(xsVar(0), xsID_idac, tmp);

		xsmcSetInteger(tmp, d->gas_wait);
		xsmcSet(xsVar(0), xsID_gas_wait, tmp);

		xsmcSetNumber(tmp, d->temperature);
		xsmcSet(xsVar(0), xsID_temperature, tmp);

		xsmcSetNumber(tmp, d->pressure);
		xsmcSet(xsVar(0), xsID_pressure, tmp);

		xsmcSetNumber(tmp, d->humidity);
		xsmcSet(xsVar(0), xsID_humidity, tmp);

		xsmcSetNumber(tmp, d->gas_resistance);
		xsmcSet(xsVar(0), xsID_gas_resistance, tmp);
	}
}

void xs_bme68x_selftest_check(xsMachine *the)
{
	BME68xlib *lib = xsmcGetHostData(xsThis);

	xsmcVars(2);

	int8_t rslt = bme68x_selftest_check(&lib->bd);
	if (BME68X_OK != rslt)
		xsUnknownError("selftest_check fail");
}

void xs_bme68x_soft_reset(xsMachine *the)
{
	BME68xlib *lib = xsmcGetHostData(xsThis);

	xsmcVars(2);

	if (0 != bme68x_soft_reset(&lib->bd))
		xsUnknownError("soft_reset fail");
}

void xs_bme68x_get_regs(xsMachine *the)
{
	BME68xlib *lib = xsmcGetHostData(xsThis);
	int reg_addr;
	void *output;
	xsUnsignedValue count;

	xsmcVars(2);

	reg_addr = xsmcToInteger(xsArg(0));
	xsmcGetBufferWritable(xsArg(1), &output, &count);

	if (0 != bme68x_get_regs(reg_addr, output, count, &lib->bd))
		xsUnknownError("get_regs fail");
}

void xs_bme68x_set_regs(xsMachine *the)
{
	BME68xlib *lib = xsmcGetHostData(xsThis);
	void *regs, *output;
	xsUnsignedValue countRegs, countOut;

	xsmcVars(2);

	xsmcGetBufferReadable(xsArg(0), &regs, &countRegs);
	xsmcGetBufferReadable(xsArg(1), &output, &countOut);
	if (countOut != countRegs)
		xsUnknownError("mismatch");

	if (0 != bme68x_set_regs(regs, output, countOut, &lib->bd))
		xsUnknownError("set_segs fail");
}
