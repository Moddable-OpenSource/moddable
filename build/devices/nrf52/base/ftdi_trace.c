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

#include "xs.h"
#include "xsPlatform.h"
#include "mc.defines.h"

#include "ftdi_trace.h"

void setupSerial();
void serial_put(uint8_t *buffer, uint32_t len);

#if USE_FTDI_TRACE 

static uint8_t ftBuf[128];

static void ftdiTx(uint8_t *buffer)
{
	serial_put(buffer, c_strlen(buffer));
}

void ftdiTraceInit()
{
	setupSerial();
	ftdiTrace("--ftdiTraceInit complete--");
}


void ftdiTrace(const char *msg)
{
	snprintf(ftBuf, sizeof(ftBuf), "%s\n", msg);
//	snprintf(ftBuf, sizeof(ftBuf), "[%p] %s\n", xTaskGetCurrentTaskHandle(), msg);
	ftdiTx(ftBuf);
}

void ftdiTrace2(const char *msg, const char *msg2)
{
	snprintf(ftBuf, sizeof(ftBuf), "%s %s\n", msg, msg2);
	ftdiTx(ftBuf);
}

void ftdiTraceAndHex(const char *msg, int i)
{
	snprintf(ftBuf, sizeof(ftBuf), "%s 0x%02x\n", msg, i);
	ftdiTx(ftBuf);
}

void ftdiTraceAndHex2(const char *msg, int i, int j)
{
	snprintf(ftBuf, sizeof(ftBuf), "%s 0x%02x 0x%02x\n", msg, i, j);
	ftdiTx(ftBuf);
}

void ftdiTraceAndInt(const char *msg, int i)
{
	snprintf(ftBuf, sizeof(ftBuf), "%s %d\n", msg, i);
//	snprintf(ftBuf, sizeof(ftBuf), "[%p] %s %d\n", xTaskGetCurrentTaskHandle(), msg, i);
	ftdiTx(ftBuf);
}

void ftdiTraceAndInt2(const char *msg, int i, int j)
{
	snprintf(ftBuf, sizeof(ftBuf), "%s %d, %d\n", msg, i, j);
	ftdiTx(ftBuf);
}

void ftdiTraceInt(int i)
{
	snprintf(ftBuf, sizeof(ftBuf), "%d\n", i);
	ftdiTx(ftBuf);
}

void ftdiTraceInt2(int i, int j)
{
	snprintf(ftBuf, sizeof(ftBuf), "%d %d\n", i, j);
	ftdiTx(ftBuf);
}

void ftdiTraceChar(int c)
{
	if (isprint(c))
		snprintf(ftBuf, sizeof(ftBuf), "%c\n", c);
	else
		snprintf(ftBuf, sizeof(ftBuf), "0x%02X\n", (uint16_t)c);
	ftdiTx(ftBuf);
}

void ftdiTraceAndAddress(uint8_t *data, void *p)
{
	snprintf(ftBuf, sizeof(ftBuf), "%s 0x%08x\n", data, p);
	ftdiTx(ftBuf);
}

void ftdiTraceAnd2Address(uint8_t *data, void *p, void *p2)
{
	snprintf(ftBuf, sizeof(ftBuf), "%s 0x%08x 0x%08x\n", data, p, p2);
	ftdiTx(ftBuf);
}

void ftdiTraceAddress(void *p)
{
	snprintf(ftBuf, sizeof(ftBuf), "0x%08x\n", p);
	ftdiTx(ftBuf);
}

void ftdiTraceHex(uint8_t *data, int num)
{
	int i, pos = 0;
	for (i=0; i<num; i++) {
		if (isprint(data[i]))
			pos += snprintf(ftBuf + pos, sizeof(ftBuf) - pos, "%c ", data[i]);
		else
			pos += snprintf(ftBuf + pos, sizeof(ftBuf) - pos, "%02x ", data[i]);
		if (i%32 == 31) {
			ftBuf[pos++] = '\n';
			ftBuf[pos++] = '\0';
			ftdiTx(ftBuf);
			pos = 0;
		}
	}
	if (i%32 != 0) {
		ftBuf[pos++] = '\n';
		ftBuf[pos++] = '\0';
		ftdiTx(ftBuf);
	}
}

void ftdiTraceAndbmReq(const char *msg, uint8_t val)
{
	int dir, type, recip;
	int pos = 0;
	dir = val >> 7;
	type = (val >> 5) & 0b11;
	recip = val & 0b1111;

	pos = snprintf(ftBuf, sizeof(ftBuf), "%s (0x%02x) %s ", msg, val, dir ? "DeviceToHost " : "HostToDevice ");
	switch (type) {
		case 0:
			pos += snprintf(ftBuf + pos, sizeof(ftBuf) - pos, "Standard "); break;
		case 1:
			pos += snprintf(ftBuf + pos, sizeof(ftBuf) - pos, "Class "); break;
		case 2:
			pos += snprintf(ftBuf + pos, sizeof(ftBuf) - pos, "Vendor "); break;
		case 3:
			pos += snprintf(ftBuf + pos, sizeof(ftBuf) - pos, "Reserved "); break;
	}
	switch (recip) {
		case 0:
			pos += snprintf(ftBuf + pos, sizeof(ftBuf) - pos, "Device "); break;
		case 1:
			pos += snprintf(ftBuf + pos, sizeof(ftBuf) - pos, "Interface "); break;
		case 2:
			pos += snprintf(ftBuf + pos, sizeof(ftBuf) - pos, "Endpoint "); break;
		case 3:
			pos += snprintf(ftBuf + pos, sizeof(ftBuf) - pos, "Other "); break;
		default:
			pos += snprintf(ftBuf + pos, sizeof(ftBuf) - pos, "Reserved "); break;
	}
	ftBuf[pos++] = '\n';
	ftBuf[pos++] = '\0';
	ftdiTx(ftBuf);
}

void ftdiTraceAndCtx(const char *msg, app_usbd_vendor_ctx_t *ctx)
{
	ftdiTrace(msg);

	snprintf(ftBuf, sizeof(ftBuf), "   - line_state: %d\n", ctx->line_state);
	ftdiTx(ftBuf);
	snprintf(ftBuf, sizeof(ftBuf), "   - rx_tran[0].read_left: %d, rx_tran[1].read_left: %d\n", ctx->rx_transfer[0].read_left, ctx->rx_transfer[1].read_left);
	ftdiTx(ftBuf);
	snprintf(ftBuf, sizeof(ftBuf), "   - copy_pos:0x%08x,  bytes_left:%d, _read:%d, last_read:%d, cur_read:%d\n", ctx->p_copy_pos, ctx->bytes_left, ctx->bytes_read, ctx->last_read, ctx->cur_read);
	ftdiTx(ftBuf);
}


#else
	void ftdiTraceInit() {}
	void ftdiTrace(const char *msg) {}
	void ftdiTrace2(const char *msg, const char *msg2) {}
	void ftdiTraceInt(int i) {}
	void ftdiTraceChar(int c) {}
	void ftdiTraceAndHex(const char *msg, int i) {}
	void ftdiTraceAndInt(const char *msg, int i) {}
	void ftdiTraceAndInt2(const char *msg, int i, int j) {}
	void ftdiTraceInt2(int i, int j) {}
	void ftdiTraceAndAddress(uint8_t *data, void *p) {}
	void ftdiTraceAnd2Address(uint8_t *data, void *p, void *p2) {}
	void ftdiTraceAddress(void *p) {}
	void ftdiTraceHex(uint8_t *data, int num) {}
	void ftdiTraceAndbmReq(const char *msg, uint8_t val) {}
	void ftdiTraceAndCtx(const char *msg, app_usbd_vendor_ctx_t *ctx) {}
#endif
