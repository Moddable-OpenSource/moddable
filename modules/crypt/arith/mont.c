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

#include "xsPlatform.h"
#include "xsmc.h"
#include "xsBigIntEx.h"

extern txBigInt *fxBigInt_mont_exp_LR(xsMachine *the, txBigInt *r, txBigInt *b, txBigInt *e, txBigInt *m);
extern txBigInt *fxBigInt_mont_exp_SW(xsMachine *the, txBigInt *r, txBigInt *b, txBigInt *e, txBigInt *m, int param);

void
xs_mont2_exp_LR(xsMachine *the)
{
	txBigInt *a, *e, *m, *r;

	a = xsmcToBigInt(xsArg(0));
	e = xsmcToBigInt(xsArg(1));
	m = xsmcToBigInt(xsArg(2));
	r = fxBigInt_mont_exp_LR(the, NULL, a, e, m);
	xsmcSetBigInt(xsResult, r);
}

void
xs_mont2_exp_SW(xsMachine *the)
{
	txBigInt *a, *e, *m, *r;
	int param;

	a = xsmcToBigInt(xsArg(0));
	e = xsmcToBigInt(xsArg(1));
	m = xsmcToBigInt(xsArg(2));
	param = xsmcToInteger(xsArg(3));
	r = fxBigInt_mont_exp_SW(the, NULL, a, e, m, param);
	xsmcSetBigInt(xsResult, r);
}
