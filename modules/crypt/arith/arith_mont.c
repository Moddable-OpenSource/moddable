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

#include "arith_common.h"
#include "kcl_arith.h"
#include "xsmc.h"

void
xs_mont_init(xsMachine *the)
{
	int ac = xsmcToInteger(xsArgc);
	mod_t *mod = xsmcGetHostData(xsThis);
	z_t *z;
	kcl_int_t *m;
	kcl_mod_method_t method = KCL_MOD_METHOD_LR;
	int options = 0;

	xsmcVars(2);
	xsmcGet(xsVar(0), xsThis, xsID_z);
	xsmcGet(xsVar(1), xsThis, xsID_m);
	z = xsmcGetHostData(xsVar(0));
	m = xsmcGetHostData(xsVar(1));
	if (ac > 0 && xsmcTypeOf(xsArg(0)) == xsIntegerType) {
		method = xsmcToInteger(xsArg(0));
		if (ac > 1 && xsmcTypeOf(xsArg(1)) == xsIntegerType)
			options = xsmcToInteger(xsArg(1));
	}
	kcl_mont_init(mod->ctx, z->ctx, m, method, options);
}
