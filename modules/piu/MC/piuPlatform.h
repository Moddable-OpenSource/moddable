/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

#include "commodettoPixelsOut.h"
#include "commodettoPocoBlit.h"
#include "commodettoStream.h"
#include "modInstrumentation.h"
#ifdef __ets__
	#include "xsHost.h"
#endif

#if COMMODETTO_BITMAP_ID
#define piuGPU 1
#endif
#define piuMC 1
typedef PocoCoordinate PiuCoordinate;
typedef PocoDimension PiuDimension;
#define xsPiuCoordinate(VALUE) xsInteger((xsIntegerValue)(VALUE))
#define xsPiuDimension(VALUE) xsInteger((xsIntegerValue)(VALUE))
#define xsToPiuCoordinate(SLOT) (PiuCoordinate)xsToInteger(SLOT)
#define xsToPiuDimension(SLOT) (PiuDimension)xsToInteger(SLOT)
