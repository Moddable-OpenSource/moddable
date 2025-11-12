/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

#include "xs.h"
#include "commodettoBitmap.h"

typedef int (*modDisplayBeginFunc)(void *hostData, int x, int y, int width, int height, void **frameBuffer, int32_t *rowBytes, int flags);
typedef int (*modDisplaySend)(void *hostData, void *buffer, uint32_t length);
typedef int (*modDisplayEndFunc)(void *hostData);
typedef void (*modDisplayAdaptInvalidFunc)(void *hostData, CommodettoRectangle r);
typedef int (*modDisplayGetFunc)(void *hostData, int32_t what, void *result);

struct xsDisplayHostHooksRecord {
	xsHostHooks							hooks;
	modDisplayBeginFunc				doBegin;
	modDisplaySend 					doSend;
	modDisplayEndFunc 				doEnd;
	modDisplayAdaptInvalidFunc 	doAdaptInvalid;
	modDisplayGetFunc 				doGet;
};

typedef struct xsDisplayHostHooksRecord xsDisplayHostHooksRecord;
typedef struct xsDisplayHostHooksRecord *xsDisplayHostHooks;
