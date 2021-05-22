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
    BMP85,BMP180 - temp/pressure
    Implementation based on Adafruit https://github.com/adafruit/Adafruit-BMP085-Library
*/
#include "xsHost.h"
#include "xsmc.h"
#include "mc.xs.h"

typedef struct calibrationSet {
	xsSlot		obj;

	int16_t		ac1;
	int16_t		ac2;
	int16_t		ac3;
	uint16_t	ac4;
	uint16_t	ac5;
	uint16_t	ac6;
	int16_t		b1;
	int16_t		b2;
	int16_t		mb;
	int16_t		mc;
	int16_t		md;
} calibrationSet, *calibrationPtr;

void xs_bmp180_host_object_constructor(xsMachine *the)
{
	calibrationPtr calib;

	calib = c_malloc(sizeof(calibrationSet));
	if (!calib)
		xsUnknownError("no memory");

	xsmcSetHostData(xsThis, calib);
	calib->obj = xsThis;
	xsRemember(xsThis);
}

void xs_bmp180_host_object_destructor(void *data)
{
	if (NULL == data)
		return;

	c_free(data);
}

void xs_bmp180_setCalibration(xsMachine *the)
{
    calibrationPtr calib = (calibrationPtr)xsmcGetHostData(xsThis);

    xsmcVars(1);

    xsmcGet(xsVar(0), xsArg(0), xsID_AC1);
	calib->ac1 = xsmcToInteger(xsVar(0));
    xsmcGet(xsVar(0), xsArg(0), xsID_AC2);
	calib->ac2 = xsmcToInteger(xsVar(0));
    xsmcGet(xsVar(0), xsArg(0), xsID_AC3);
	calib->ac3 = xsmcToInteger(xsVar(0));
    xsmcGet(xsVar(0), xsArg(0), xsID_AC4);
	calib->ac4 = xsmcToInteger(xsVar(0));
    xsmcGet(xsVar(0), xsArg(0), xsID_AC5);
	calib->ac5 = xsmcToInteger(xsVar(0));
    xsmcGet(xsVar(0), xsArg(0), xsID_AC6);
	calib->ac6 = xsmcToInteger(xsVar(0));
    xsmcGet(xsVar(0), xsArg(0), xsID_B1);
	calib->b1 = xsmcToInteger(xsVar(0));
    xsmcGet(xsVar(0), xsArg(0), xsID_B2);
	calib->b2 = xsmcToInteger(xsVar(0));
    xsmcGet(xsVar(0), xsArg(0), xsID_MB);
	calib->mb = xsmcToInteger(xsVar(0));
    xsmcGet(xsVar(0), xsArg(0), xsID_MC);
	calib->mc = xsmcToInteger(xsVar(0));
    xsmcGet(xsVar(0), xsArg(0), xsID_MD);
	calib->md = xsmcToInteger(xsVar(0));
}

void xs_bmp180_close(xsMachine *the)
{
	calibrationPtr calib = xsmcGetHostData(xsThis);
	if (NULL == calib)
		return;
}

int computeB5(uint32_t UT, calibrationPtr calib)
{
	int32_t X1 = (UT - (int32_t)calib->ac6) * ((int32_t)calib->ac5) >> 15;
	int32_t X2 = ((int32_t)calib->mc << 11) / (X1 + (int32_t)calib->md);
	return X1 + X2;
}

void xs_bmp180_calculate(xsMachine *the)
{
	calibrationPtr calib = xsmcGetHostData(xsThis);
	if (NULL == calib)
		return;

	int32_t UT, B5;
	double T;

	UT = xsmcToInteger(xsArg(0));
	B5 = computeB5(UT, calib);
	T = (B5 + 8) >> 4;
	T /= 10.0;

	xsResult = xsNewHostObject(c_free);
	xsmcVars(1);
	xsVar(0) = xsNumber(T);
	xsmcSet(xsResult, xsID_temperature, xsVar(0));

	int32_t UP, B3, B6, X1, X2, X3, p, mode;
	uint32_t B4, B7;

	UP = xsmcToInteger(xsArg(1));
	mode = xsmcToInteger(xsArg(2));
	B6 = B5 - 4000;
	X1 = ((int32_t)calib->b2 * ((B6 * B6) >> 12)) >> 11;
	X2 = ((int32_t)calib->ac2 * B6) >> 11;
	X3 = X1 + X2;
	B3 = ((((int32_t)calib->ac1 * 4 + X3) << mode) + 2) / 4;

	X1 = ((int32_t)calib->ac3 * B6) >> 13;
	X2 = ((int32_t)calib->b1 * ((B6 * B6) >> 12)) >> 16;
	X3 = ((X1 + X2) + 2) >> 2;
	B4 = ((uint32_t)calib->ac4 * (uint32_t)(X3 + 32768)) >> 15;
	B7 = ((uint32_t)UP - B3) * (uint32_t)(50000UL >> mode);

	if (B7 < 0x80000000)
		p = (B7 * 2) / B4;
	else
		p = (B7 / B4) * 2;
	X1 = (p >> 8) * (p >> 8);
	X1 = (X1 * 3038) >> 16;
	X2 = (-7357 * p) >> 16;

	p = p + ((X1 + X2 + (int32_t)3791) >> 4);

	xsVar(0) = xsInteger(p);
	xsmcSet(xsResult, xsID_pressure, xsVar(0));
}

