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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

#ifndef __XSALL__
typedef struct {
	txU4 opaque;
}  txBigInt;
typedef xsBooleanValue txBoolean;

extern void fxBigInt_setBigInt(xsSlot *slot, txBigInt *a);

#define _OVERHEAD	(sizeof(void *) * 2)
#define xsmcToBigInt(_SLOT)	(txBigInt *)(((char *)&(_SLOT)) + _OVERHEAD)
#define xsmcBigInt(a)	(xsSlot *)(((char *)(a)) - _OVERHEAD)
#define xsmcSetBigInt(_SLOT, a)	fxBigInt_setBigInt(&_SLOT, a)
#endif

typedef struct sxECParam {
	txBigInt *a;
	txBigInt *b;
	txBigInt *m;
	txU4 u;
	txBigInt *one;
} txECParam;

typedef struct sxECPoint {
	txBigInt *x;
	txBigInt *y;
	txBigInt *z;
} txECPoint;
