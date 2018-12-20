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

#define piuPC 1
typedef int32_t PiuCoordinate;
typedef int32_t PiuDimension;

#ifdef mxDebug
	#define xsThrowErrno(_ASSERTION) \
		(fxThrowErrno(the,(char *)__FILE__,__LINE__))
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (fxThrowErrno(the,(char *)__FILE__,__LINE__), 1)))
#else
	#define xsThrowErrno(_ASSERTION) \
		(fxThrowErrno(the,NULL,0))
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (fxThrowErrno(the,NULL,0), 1)))
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern void fxThrowErrno(xsMachine* the, xsStringValue path, xsIntegerValue line);
#ifdef __cplusplus
}
#endif
