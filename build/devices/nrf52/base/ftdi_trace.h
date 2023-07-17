/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

#include <stdint.h>

#ifndef _FTDI_TRACE_
#define _FTDI_TRACE_

#include "app_usbd_vendor.h"

void ftdiTraceInit();
void ftdiTrace(const char *msg);
void ftdiTrace2(const char *msg, const char *msg2);
void ftdiTraceInt(int i);
void ftdiTraceInt2(int i, int j);
void ftdiTraceChar(int c);
void ftdiTraceHex(uint8_t *data, int num);
void ftdiTraceAndHex(const char *msg, int i);
void ftdiTraceAndHex2(const char *msg, int i, int j);
void ftdiTraceAndInt(const char *msg, int i);
void ftdiTraceAndInt2(const char *msg, int i, int j);
void ftdiTraceAndbmReq(const char *msg, uint8_t val);
void ftdiTraceAndCtx(const char *msg, app_usbd_vendor_ctx_t *ctx);

#endif
