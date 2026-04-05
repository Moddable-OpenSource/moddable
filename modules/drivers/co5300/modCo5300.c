/*
 * Copyright (c) 2024-2026  Moddable Tech, Inc.
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
	CO5300 QSPI AMOLED display driver
	Used by Waveshare ESP32-S3-Touch-AMOLED-2.06 and similar boards
	Communication via QSPI (quad SPI) using ESP-IDF esp_lcd panel IO
*/

#include "xsmc.h"
#include "xsHost.h"

#include "commodettoBitmap.h"
#include "commodettoPocoBlit.h"
#include "commodettoPixelsOut.h"
#include "mc.xs.h"
#include "mc.defines.h"

#include "modGPIO.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_lcd_types.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_io.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#if !defined(MODDEF_CO5300_CS_PIN) || !defined(MODDEF_CO5300_SCK_PIN)
	#error CO5300 CS and SCK pins must be defined
#endif
#if !defined(MODDEF_CO5300_DATA0_PIN) || !defined(MODDEF_CO5300_DATA1_PIN) || !defined(MODDEF_CO5300_DATA2_PIN) || !defined(MODDEF_CO5300_DATA3_PIN)
	#error CO5300 QSPI data pins (DATA0-DATA3) must be defined
#endif

#ifndef MODDEF_CO5300_HZ
	#define MODDEF_CO5300_HZ (40000000)
#endif
#ifndef MODDEF_CO5300_COLUMN_OFFSET
	#define MODDEF_CO5300_COLUMN_OFFSET 22
#endif
#ifndef MODDEF_CO5300_ROW_OFFSET
	#define MODDEF_CO5300_ROW_OFFSET 0
#endif
#ifndef MODDEF_CO5300_SPI_PORT
	#define MODDEF_CO5300_SPI_PORT SPI2_HOST
#endif

#define CO5300_OPQUEUE (10)

/*
	QSPI command encoding for CO5300/SH8601:
	Bits 31-24: Instruction byte (0x02 = write, 0x32 = quad data write)
	Bits 23-16: Address high byte (0x00 for single-byte registers)
	Bits 15-8:  Address low byte (register address)
	Bits 7-0:   Dummy byte (0x00)
*/
#define CO5300_CMD(reg)      ((0x02 << 24) | ((reg) << 8))
#define CO5300_COLOR_CMD     ((0x32 << 24) | (0x2C << 8))

typedef struct {
	PixelsOutDispatch			dispatch;

#ifdef MODDEF_CO5300_RST_PIN
	modGPIOConfigurationRecord	rst;
#endif

	int updateWidth;
	int updateLinesRemaining;
	int yMin;
	int yMax;
	int ping;
	uint8_t nothingSent;
	uint8_t firstFrame;
	uint8_t brightnessValue;

	SemaphoreHandle_t			colorsInFlight;
	esp_lcd_panel_io_handle_t	io_handle;

	QueueHandle_t				ops;
	int							opZero;

	uint8_t data[32];
} co5300DisplayRecord, *co5300Display;

static void co5300Init(co5300Display sd);

#define co5300Command(sd, command, data, count) \
	(esp_lcd_panel_io_tx_param(sd->io_handle, command, data, count))

#define co5300CommandAsync(sd, command, data, count) \
	xQueueSend(sd->ops, &sd->opZero, portMAX_DELAY); \
	esp_lcd_panel_io_tx_color(sd->io_handle, command, data, count)

static void co5300Begin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h);
static void co5300End(void *refcon);
static void co5300Continue(void *refcon);
static void co5300Send(PocoPixel *pixels, int byteLength, void *refcon);
static void co5300AdaptInvalid(void *refcon, CommodettoRectangle r);

static const PixelsOutDispatchRecord gPixelsOutDispatch ICACHE_RODATA_ATTR = {
	co5300Begin,
	co5300Continue,
	co5300End,
	co5300Send,
	co5300AdaptInvalid
};

void xs_co5300_destructor(void *data)
{
	co5300Display sd = data;
	if (!data) return;

	if (sd->io_handle)
		esp_lcd_panel_io_del(sd->io_handle);

#ifdef MODDEF_CO5300_RST_PIN
	modGPIOUninit(&sd->rst);
#endif

	if (sd->ops)
		vQueueDelete(sd->ops);

	if (sd->colorsInFlight)
		vSemaphoreDelete(sd->colorsInFlight);

	c_free(data);
}

static bool colorDone(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
	co5300Display sd = user_ctx;
	BaseType_t high_task_woken = pdFALSE, doYield = pdFALSE;
	int op;

	if (pdTRUE == xQueueReceiveFromISR(sd->ops, &op, &high_task_woken)) {
		if (op)
			xSemaphoreGiveFromISR(sd->colorsInFlight, &doYield);
	}

	return doYield || high_task_woken;
}

void xs_co5300(xsMachine *the)
{
	co5300Display sd;

	if (xsmcHas(xsArg(0), xsID_pixelFormat)) {
		xsmcVars(1);
		xsmcGet(xsVar(0), xsArg(0), xsID_pixelFormat);
		if (kCommodettoBitmapFormat != xsmcToInteger(xsVar(0)))
			xsUnknownError("bad format");
	}

	sd = c_calloc(1, sizeof(co5300DisplayRecord));
	if (!sd)
		xsUnknownError("no memory");

	xsmcSetHostData(xsThis, sd);

	spi_bus_config_t buscfg = {
		.sclk_io_num = MODDEF_CO5300_SCK_PIN,
		.data0_io_num = MODDEF_CO5300_DATA0_PIN,
		.data1_io_num = MODDEF_CO5300_DATA1_PIN,
		.data2_io_num = MODDEF_CO5300_DATA2_PIN,
		.data3_io_num = MODDEF_CO5300_DATA3_PIN,
		.max_transfer_sz = MODDEF_CO5300_WIDTH * 80 * sizeof(uint16_t),
		.flags = SPICOMMON_BUSFLAG_MASTER,
	};

	int err = spi_bus_initialize(MODDEF_CO5300_SPI_PORT, &buscfg, SPI_DMA_CH_AUTO);
	if (err) {
		c_free(sd);
		xsmcSetHostData(xsThis, NULL);
		xsUnknownError("spi_bus_initialize failed");
	}

	esp_lcd_panel_io_spi_config_t io_config = {
		.cs_gpio_num = MODDEF_CO5300_CS_PIN,
		.dc_gpio_num = -1,
		.spi_mode = 0,
		.pclk_hz = MODDEF_CO5300_HZ,
		.trans_queue_depth = CO5300_OPQUEUE,
		.on_color_trans_done = colorDone,
		.user_ctx = sd,
		.lcd_cmd_bits = 32,
		.lcd_param_bits = 8,
		.flags = {
			.quad_mode = true,
		},
	};

	err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)MODDEF_CO5300_SPI_PORT, &io_config, &sd->io_handle);
	if (err) {
		spi_bus_free(MODDEF_CO5300_SPI_PORT);
		c_free(sd);
		xsmcSetHostData(xsThis, NULL);
		xsUnknownError("esp_lcd_new_panel_io_spi failed");
	}

	sd->dispatch = (PixelsOutDispatch)&gPixelsOutDispatch;

#ifdef MODDEF_CO5300_RST_PIN
	modGPIOInit(&sd->rst, NULL, MODDEF_CO5300_RST_PIN, kModGPIOOutput);
	modGPIOWrite(&sd->rst, 1);
#endif

	sd->ops = xQueueCreate(CO5300_OPQUEUE, sizeof(int));
	sd->opZero = 0;

	sd->colorsInFlight = xSemaphoreCreateCounting(2, 2);

	co5300Init(sd);
}

void xs_co5300_begin(xsMachine *the)
{
	co5300Display sd = xsmcGetHostData(xsThis);
	CommodettoCoordinate x = (CommodettoCoordinate)xsmcToInteger(xsArg(0));
	CommodettoCoordinate y = (CommodettoCoordinate)xsmcToInteger(xsArg(1));
	CommodettoDimension w = (CommodettoDimension)xsmcToInteger(xsArg(2));
	CommodettoDimension h = (CommodettoDimension)xsmcToInteger(xsArg(3));

	co5300Begin(sd, x, y, w, h);
}

void xs_co5300_send(xsMachine *the)
{
	co5300Display sd = xsmcGetHostData(xsThis);
	int argc = xsmcArgc;
	const uint8_t *data;
	xsUnsignedValue count;

	xsmcGetBufferReadable(xsArg(0), (void **)&data, &count);

	if (argc > 1) {
		xsIntegerValue offset = xsmcToInteger(xsArg(1));

		if ((xsUnsignedValue)offset >= count)
			xsUnknownError("bad offset");
		data += offset;
		count -= offset;
		if (argc > 2) {
			xsIntegerValue c = xsmcToInteger(xsArg(2));
			if (c > count)
				xsUnknownError("bad count");
			count = c;
		}
	}

	co5300Send((PocoPixel *)data, count, sd);
}

void xs_co5300_end(xsMachine *the)
{
	co5300Display sd = xsmcGetHostData(xsThis);
	co5300End(sd);
}

void xs_co5300_continue(xsMachine *the)
{
	co5300Display sd = xsmcGetHostData(xsThis);
	co5300Continue(sd);
}

void xs_co5300_pixelsToBytes(xsMachine *the)
{
	int count = xsmcToInteger(xsArg(0));
	xsmcSetInteger(xsResult, ((count * kCommodettoPixelSize) + 7) >> 3);
}

void xs_co5300_get_pixelFormat(xsMachine *the)
{
	xsmcSetInteger(xsResult, kCommodettoBitmapFormat);
}

void xs_co5300_get_width(xsMachine *the)
{
	xsmcSetInteger(xsResult, MODDEF_CO5300_WIDTH);
}

void xs_co5300_get_height(xsMachine *the)
{
	xsmcSetInteger(xsResult, MODDEF_CO5300_HEIGHT);
}

void xs_co5300_get_c_dispatch(xsMachine *the)
{
	xsResult = xsThis;
}

void xs_co5300_command(xsMachine *the)
{
	co5300Display sd = xsmcGetHostData(xsThis);
	uint8_t command = (uint8_t)xsmcToInteger(xsArg(0));
	xsUnsignedValue dataSize = 0;
	uint8_t *data = NULL;

	if (xsmcArgc > 1)
		xsmcGetBufferReadable(xsArg(1), (void **)&data, &dataSize);

	co5300Command(sd, CO5300_CMD(command), data, (uint16_t)dataSize);
}

void xs_co5300_set_brightness(xsMachine *the)
{
	co5300Display sd = xsmcGetHostData(xsThis);
	int percent = xsmcToInteger(xsArg(0));
	if (percent < 0) percent = 0;
	if (percent > 100) percent = 100;

	sd->brightnessValue = (uint8_t)(percent * 255 / 100);
	uint8_t param = sd->brightnessValue;
	co5300Command(sd, CO5300_CMD(0x51), &param, 1);
}

void xs_co5300_get_brightness(xsMachine *the)
{
	co5300Display sd = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, sd->brightnessValue * 100 / 255);
}

void xs_co5300_close(xsMachine *the)
{
	co5300Display sd = xsmcGetHostData(xsThis);
	if (!sd) return;
	xs_co5300_destructor(sd);
	xsmcSetHostData(xsThis, NULL);
}

void co5300Send(PocoPixel *pixels, int byteLength, void *refcon)
{
	co5300Display sd = refcon;
	uint8_t sync = byteLength > 0;
	if (!sync)
		byteLength = -byteLength;

	sd->nothingSent = 0;

#if kCommodettoBitmapFormat == kCommodettoBitmapRGB565LE
	{
		uint16_t *p = (uint16_t *)pixels;
		int count = byteLength >> 1;
		for (int i = 0; i < count; i++)
			p[i] = __builtin_bswap16(p[i]);
	}
#endif

	{
		int one = 1;
		xQueueSend(sd->ops, &one, portMAX_DELAY);
		esp_lcd_panel_io_tx_color(sd->io_handle, CO5300_COLOR_CMD, pixels, byteLength);

		int lines = (byteLength >> 1) / sd->updateWidth;
		sd->yMin += lines;
		sd->updateLinesRemaining -= lines;

		if (sd->updateLinesRemaining) {
			uint8_t data[4];
			data[0] = (sd->yMin >> 8) & 0xff;
			data[1] = sd->yMin & 0xff;
			data[2] = (sd->yMax >> 8) & 0xff;
			data[3] = sd->yMax & 0xff;
			co5300Command(sd, CO5300_CMD(0x2b), data, 4);
		}
	}

	if (sync) {
		xSemaphoreTake(sd->colorsInFlight, portMAX_DELAY);
		xSemaphoreTake(sd->colorsInFlight, portMAX_DELAY);
		xSemaphoreGive(sd->colorsInFlight);
		xSemaphoreGive(sd->colorsInFlight);
	}
	else if (sd->updateLinesRemaining)
		xSemaphoreTake(sd->colorsInFlight, portMAX_DELAY);
}

static const uint8_t gInit[] ICACHE_RODATA_ATTR = {
	0x11, 0,				// Sleep Out
	0xFF, 120,				// delay 120ms

	0xFE, 1, 0x00,			// Page select (page 0)
	0xC4, 1, 0x80,			// Set interface (QSPI)
	0x3A, 1, 0x55,			// Pixel format: 16-bit RGB565
	0x44, 2, 0x01, 0xD1,	// Set tear scanline
	0x35, 1, 0x00,			// Tearing effect line on
	0x53, 1, 0x20,			// Write CTRL display value
	0x63, 1, 0xFF,			// HBM brightness
	0x51, 1, 0x00,			// Display brightness (off during init)
	0x58, 1, 0x00,			// Contrast enhancement off

	0xFF, 0					// end
};

void co5300Init(co5300Display sd)
{
	const uint8_t *cmds;

#ifdef MODDEF_CO5300_RST_PIN
	modGPIOWrite(&sd->rst, 0);
	modDelayMilliseconds(20);
	modGPIOWrite(&sd->rst, 1);
	modDelayMilliseconds(200);
#endif

	cmds = gInit;
	while (true) {
		uint8_t cmd = c_read8(cmds++);
		if (0xFF == cmd) {
			uint8_t ms = c_read8(cmds++);
			if (0 == ms)
				break;
			modDelayMilliseconds(ms);
		}
		else {
			uint8_t count = c_read8(cmds++);
			co5300Command(sd, CO5300_CMD(cmd), cmds, count);
			cmds += count;
		}
	}

	sd->firstFrame = true;
	sd->brightnessValue = 0;
}

void co5300AdaptInvalid(void *refcon, CommodettoRectangle r)
{
	// CO5300 requires even-aligned coordinates for partial updates
	CommodettoCoordinate x2 = r->x + r->w;
	CommodettoCoordinate y2 = r->y + r->h;

	r->x &= ~1;
	r->y &= ~1;
	x2 = (x2 + 1) & ~1;
	y2 = (y2 + 1) & ~1;

	r->w = x2 - r->x;
	r->h = y2 - r->y;
}

void co5300Begin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h)
{
	co5300Display sd = refcon;
	uint16_t xMin, xMax, yMin, yMax;

	if (sd->nothingSent)
		xSemaphoreGive(sd->colorsInFlight);
	sd->nothingSent = 1;

	xMin = x + MODDEF_CO5300_COLUMN_OFFSET;
	yMin = y + MODDEF_CO5300_ROW_OFFSET;

	xMax = xMin + w - 1;
	yMax = yMin + h - 1;

	sd->updateWidth = w;
	sd->updateLinesRemaining = h;
	sd->yMin = yMin;
	sd->yMax = yMax;

	uint8_t data[4];
	data[0] = (xMin >> 8) & 0xff;
	data[1] = xMin & 0xff;
	data[2] = (xMax >> 8) & 0xff;
	data[3] = xMax & 0xff;
	co5300Command(sd, CO5300_CMD(0x2a), data, 4);

	data[0] = (yMin >> 8) & 0xff;
	data[1] = yMin & 0xff;
	data[2] = (yMax >> 8) & 0xff;
	data[3] = yMax & 0xff;
	co5300Command(sd, CO5300_CMD(0x2b), data, 4);

	xSemaphoreTake(sd->colorsInFlight, portMAX_DELAY);
}

void co5300Continue(void *refcon)
{
}

void co5300End(void *refcon)
{
	co5300Display sd = refcon;

	if (sd->firstFrame) {
		sd->firstFrame = false;

		co5300CommandAsync(sd, CO5300_CMD(0x29), NULL, 0);	// Display ON

		sd->brightnessValue = 255;
		uint8_t bright = 0xFF;
		co5300Command(sd, CO5300_CMD(0x51), &bright, 1);
	}

	if (sd->nothingSent) {
		xSemaphoreGive(sd->colorsInFlight);
		sd->nothingSent = 0;
	}
}
