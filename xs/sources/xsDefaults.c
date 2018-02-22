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

#include "xsAll.h"

const txDefaults ICACHE_FLASH_ATTR gxDefaults  = {
	fxNewAsyncInstance,
	fxRunAsync,
	fxRunAwait,
	fxNewGeneratorInstance,
	fxNewGeneratorFunctionInstance,
	fxRunEvalEnvironment,
	fxRunProgramEnvironment,
};

const txBehavior* ICACHE_RAM_ATTR gxBehaviors[XS_BEHAVIOR_COUNT]  = {
	&gxOrdinaryBehavior,
#ifdef mxSloppy
	&gxArgumentsSloppyBehavior,
#else
	C_NULL,
#endif
	&gxArgumentsStrictBehavior,
	&gxArrayBehavior,
	&gxEnvironmentBehavior,
	&gxGlobalBehavior,
	&gxModuleBehavior,
	&gxProxyBehavior,
	&gxStringBehavior,
	&gxTypedArrayBehavior
};

const txTypeDispatch ICACHE_FLASH_ATTR gxTypeDispatches[mxTypeArrayCount] = {
	{ 4, fxFloat32Getter, fxFloat32Setter, fxFloat32Compare, _getFloat32, _setFloat32, _Float32Array },
	{ 8, fxFloat64Getter, fxFloat64Setter, fxFloat64Compare, _getFloat64, _setFloat64, _Float64Array },
	{ 1, fxInt8Getter, fxInt8Setter, fxInt8Compare, _getInt8, _setInt8, _Int8Array },
	{ 2, fxInt16Getter, fxInt16Setter, fxInt16Compare, _getInt16, _setInt16, _Int16Array },
	{ 4, fxInt32Getter, fxInt32Setter, fxInt32Compare, _getInt32, _setInt32, _Int32Array },
	{ 1, fxUint8Getter, fxUint8Setter, fxUint8Compare, _getUint8, _setUint8, _Uint8Array },
	{ 2, fxUint16Getter, fxUint16Setter, fxUint16Compare, _getUint16, _setUint16, _Uint16Array },
	{ 4, fxUint32Getter, fxUint32Setter, fxUint32Compare, _getUint32, _setUint32, _Uint32Array },
	{ 1, fxUint8Getter, fxUint8ClampedSetter, fxUint8Compare, _getUint8Clamped, _setUint8Clamped, _Uint8ClampedArray }
};

const txTypeAtomics ICACHE_FLASH_ATTR gxTypeAtomics[mxTypeArrayCount] = {
	{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },
	{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },
	{ fxInt8Add, fxInt8And, fxInt8CompareExchange, fxInt8Exchange, fxInt8Load, fxInt8Or, fxInt8Store, fxInt8Sub, fxInt8Xor },
	{ fxInt16Add, fxInt16And, fxInt16CompareExchange, fxInt16Exchange, fxInt16Load, fxInt16Or, fxInt16Store, fxInt16Sub, fxInt16Xor },
	{ fxInt32Add, fxInt32And, fxInt32CompareExchange, fxInt32Exchange, fxInt32Load, fxInt32Or, fxInt32Store, fxInt32Sub, fxInt32Xor },
	{ fxUint8Add, fxUint8And, fxUint8CompareExchange, fxUint8Exchange, fxUint8Load, fxUint8Or, fxUint8Store, fxUint8Sub, fxUint8Xor },
	{ fxUint16Add, fxUint16And, fxUint16CompareExchange, fxUint16Exchange, fxUint16Load, fxUint16Or, fxUint16Store, fxUint16Sub, fxUint16Xor },
	{ fxUint32Add, fxUint32And, fxUint32CompareExchange, fxUint32Exchange, fxUint32Load, fxUint32Or, fxUint32Store, fxUint32Sub, fxUint32Xor },
	{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL }
};
