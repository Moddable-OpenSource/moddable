/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
    BMP280 - temp/pressure
    Implementation based on Adafruit https://github.com/adafruit/Adafruit_BMP280_Library
*/
#include "xsHost.h"
#include "xsmc.h"
#include "mc.xs.h"

typedef struct calibrationSet {
	xsSlot		obj;
	xsMachine	*the;

	uint16_t	T1;
	int16_t		T2;
	int16_t		T3;

	uint16_t	P1;
	int16_t		P2;
	int16_t		P3;
	int16_t		P4;
	int16_t		P5;
	int16_t		P6;
	int16_t		P7;
	int16_t		P8;
	int16_t		P9;
} calibrationSet, *calibrationPtr;

void xs_bmp280_host_object_constructor(xsMachine *the)
{
	calibrationPtr calib;

	calib = c_malloc(sizeof(calibrationSet));
	if (!calib)
		xsUnknownError("no memory");

	xsmcSetHostData(xsThis, calib);
	calib->obj = xsThis;
	calib->the = the;
	xsRemember(xsThis);
}

void xs_bmp280_host_object_destructor(void *data)
{
	if (NULL == data)
		return;

	c_free(data);
}

void xs_bmp280_setCalibration(xsMachine *the)
{
	calibrationPtr calib = (calibrationPtr)xsmcGetHostData(xsThis);

	xsmcVars(1);

	xsmcGet(xsVar(0), xsArg(0), xsID_T1);
	calib->T1 = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_T2);
	calib->T2 = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_T3);
	calib->T3 = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_P1);
	calib->P1 = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_P2);
	calib->P2 = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_P3);
	calib->P3 = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_P4);
	calib->P4 = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_P5);
	calib->P5 = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_P6);
	calib->P6 = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_P7);
	calib->P7 = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_P8);
	calib->P8 = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_P9);
	calib->P9 = xsmcToInteger(xsVar(0));
}

void xs_bmp280_close(xsMachine *the)
{
	calibrationPtr calib = xsmcGetHostData(xsThis);
	if (NULL == calib)
		return;

	xsForget(calib->obj);
	xsmcSetHostData(xsThis, NULL);
	c_free(calib);
}

void xs_bmp280_calculate(xsMachine *the)
{
	calibrationPtr calib = xsmcGetHostData(xsThis);
	if (NULL == calib)
		return;

	int32_t var1, var2;
	int32_t adc_T = xsmcToInteger(xsArg(0));
	int32_t	t_fine;

	adc_T >>= 4;
	var1 = ((((adc_T >> 3) - ((int32_t)calib->T1 << 1))) *
		((int32_t)calib->T2)) >> 11;

	var2 = (((((adc_T >> 4) - ((int32_t)calib->T1)) *
			((adc_T >> 4) - ((int32_t)calib->T1))) >> 12) *
			((int32_t)calib->T3)) >> 14;

	t_fine = var1 + var2;

	double T = ((t_fine * 5 + 128) >> 8);
	T /= 100.0;

	xsResult = xsNewHostObject(c_free);
	xsmcVars(1);
	xsVar(0) = xsNumber(T);
	xsmcSet(xsResult, xsID_temperature, xsVar(0));

	int64_t pvar1, pvar2, p;

	int32_t adc_P = xsmcToInteger(xsArg(1));
	adc_P >>= 4;

	pvar1 = ((int64_t)t_fine) - 128000;
	pvar2 = pvar1 * pvar1 * (int64_t)calib->P6;
	pvar2 = pvar2 + ((pvar1 * (int64_t)calib->P5) << 17);
	pvar2 = pvar2 + (((int64_t)calib->P4) << 35);
	pvar1 = ((pvar1 * pvar1 * (int64_t)calib->P3) >> 8) +
			((pvar1 * (int64_t)calib->P2) << 12);
	pvar1 = (((((int64_t)1) << 47) + pvar1)) * ((int64_t)calib->P1) >> 33;

	if (pvar1 == 0)
		xsUnknownError("bmp280 div by zero");

	p = 1048576 - adc_P;
	p = (((p << 31) - pvar2) * 3125) / pvar1;
	pvar1 = (((int64_t)calib->P9) * (p >> 13) * (p >> 13)) >> 25;
	pvar2 = (((int64_t)calib->P8) * p) >> 19;

	p = ((p + pvar1 + pvar2) >> 8) + (((int64_t)calib->P7) << 4);

	double P = (double)p / 256.0;

	xsVar(0) = xsNumber(P);
	xsmcSet(xsResult, xsID_pressure, xsVar(0));
}

