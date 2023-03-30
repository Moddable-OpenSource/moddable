/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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
	to do:
		update tearing effect support 
*/

#include "xsmc.h"
#include "xsHost.h"

#include "commodettoBitmap.h"
#include "commodettoPocoBlit.h"
#include "commodettoPixelsOut.h"
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#include "modGPIO.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_lcd_types.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_io.h"

#include "driver/gpio.h"

#if !defined(MODDEF_ILI9341P8_DC_PIN) || !defined(MODDEF_ILI9341P8_CS_PIN) || !defined(MODDEF_ILI9341P8_PCLK_PIN)
	#error required pin not defined
#endif
#if !defined(MODDEF_ILI9341P8_DATA0_PIN) || !defined(MODDEF_ILI9341P8_DATA1_PIN) || !defined(MODDEF_ILI9341P8_DATA2_PIN) || !defined(MODDEF_ILI9341P8_DATA3_PIN) || !defined(MODDEF_ILI9341P8_DATA4_PIN) || !defined(MODDEF_ILI9341P8_DATA5_PIN) || !defined(MODDEF_ILI9341P8_DATA6_PIN) || !defined(MODDEF_ILI9341P8_DATA7_PIN)
	#error required data pin not defined
#endif
#ifndef MODDEF_ILI9341P8_HZ
	#define MODDEF_ILI9341P8_HZ (20000000)
#endif
#ifndef MODDEF_ILI9341P8_FLIPX
	#define MODDEF_ILI9341P8_FLIPX (false)
#endif
#ifndef MODDEF_ILI9341P8_FLIPY
	#define MODDEF_ILI9341P8_FLIPY (false)
#endif
#ifndef MODDEF_ILI9341P8_BACKLIGHT_ON
	#define MODDEF_ILI9341P8_BACKLIGHT_ON (0)
#endif
#if MODDEF_ILI9341P8_BACKLIGHT_ON
	#define MODDEF_ILI9341P8_BACKLIGHT_OFF (0)
#else
	#define MODDEF_ILI9341P8_BACKLIGHT_OFF (1)
#endif
#ifndef MODDEF_ILI9341P8_COLUMN_OFFSET
	#define MODDEF_ILI9341P8_COLUMN_OFFSET 0
#endif
#ifndef MODDEF_ILI9341P8_ROW_OFFSET
	#define MODDEF_ILI9341P8_ROW_OFFSET 0
#endif
#ifndef ILI9341_GRAM_WIDTH
	#define ILI9341_GRAM_WIDTH		MODDEF_ILI9341P8_WIDTH
#endif
#ifndef ILI9341_GRAM_HEIGHT
	#define ILI9341_GRAM_HEIGHT		MODDEF_ILI9341P8_HEIGHT
#endif
#define ILI9341_GRAM_X_OFFSET  ILI9341_GRAM_WIDTH - MODDEF_ILI9341P8_WIDTH
#define ILI9341_GRAM_Y_OFFSET  ILI9341_GRAM_HEIGHT - MODDEF_ILI9341P8_HEIGHT

#define MODDEF_ILI9341P8_OPQUEUE (10)

typedef struct {
	PixelsOutDispatch			dispatch;

#ifdef MODDEF_ILI9341P8_RST_PIN
	modGPIOConfigurationRecord	rst;
#endif
#ifdef MODDEF_ILI9341P8_BACKLIGHT_PIN
	modGPIOConfigurationRecord	backlight;
#endif
#ifdef MODDEF_ILI9341P8_READ_PIN
	modGPIOConfigurationRecord	readEn;
#endif
#ifdef MODDEF_ILI9341P8_TEARINGEFFECT_PIN
	modGPIOConfigurationRecord	tearingEffect;
	uint32_t					te_byteLength;
	volatile void				*te_pixels;
#endif

	int updateWidth;
	int updateLinesRemaining;
	int yMin;
	int yMax;
	int ping;

	uint8_t						firstFrame;
	uint8_t						firstBuffer;
	uint8_t						isContinue;
	uint8_t						memoryAccessControl;	// register 36h initialization value

	SemaphoreHandle_t			colorsInFlight;
	esp_lcd_panel_io_handle_t	io_handle;
	esp_lcd_i80_bus_handle_t	i80_bus_handle;

    QueueHandle_t				ops;
    int							opZero;		// async commnd is zero

	uint8_t data[32];
} spiDisplayRecord, *spiDisplay;

static void ili9341Init(spiDisplay sd);

// note that sync and async are very different:
//	- async needs data to remain available until completed
//	- async data needs to be endian flipped when swap_color_bytes is enabled
#define ili9341Command(sd, command, data, count) (esp_lcd_panel_io_tx_param(sd->io_handle, command, data, count))
#define ili9341CommandAsync(sd, command, data, count) \
	xQueueSend(sd->ops, &sd->opZero, portMAX_DELAY); \
	esp_lcd_panel_io_tx_color(sd->io_handle, command, data, count)

static void ili9341Begin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h);
static void ili9341End(void *refcon);
static void ili9341Continue(void *refcon);

#ifdef MODDEF_ILI9341P8_TEARINGEFFECT_PIN
	static void tearingEffectISR(void *refcon);
#endif

static void ili9341Send(PocoPixel *pixels, int byteLength, void *refcon);
static const PixelsOutDispatchRecord gPixelsOutDispatch ICACHE_RODATA_ATTR = {
	ili9341Begin,
	ili9341Continue,
	ili9341End,
	ili9341Send,
	NULL
};

void xs_ILI9341p8_destructor(void *data)
{
	spiDisplay sd = data;
	if (!data) return;

	if (sd->io_handle)
		esp_lcd_panel_io_del(sd->io_handle);

	if (sd->i80_bus_handle)
		esp_lcd_del_i80_bus(sd->i80_bus_handle);

#ifdef MODDEF_ILI9341P8_RST_PIN
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
	spiDisplay sd = user_ctx;
	BaseType_t high_task_woken = pdFALSE, doYield = pdFALSE;
	int op;

	xQueueReceiveFromISR(sd->ops, &op, &high_task_woken);
	if (op)
		xSemaphoreGiveFromISR(sd->colorsInFlight, &doYield);

	return doYield || high_task_woken;
}

void xs_ILI9341p8(xsMachine *the)
{
	spiDisplay sd;

	if (xsmcHas(xsArg(0), xsID_pixelFormat)) {
		xsmcVars(1);
		xsmcGet(xsVar(0), xsArg(0), xsID_pixelFormat);
		if (kCommodettoBitmapFormat != xsmcToInteger(xsVar(0)))
			xsUnknownError("bad format");
	}

	sd = c_calloc(1, sizeof(spiDisplayRecord));
	if (!sd)
		xsUnknownError("no memory");

	xsmcSetHostData(xsThis, sd);

	int err;
    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = MODDEF_ILI9341P8_DC_PIN,
        .wr_gpio_num = MODDEF_ILI9341P8_PCLK_PIN,
        .data_gpio_nums = {
            MODDEF_ILI9341P8_DATA0_PIN,
            MODDEF_ILI9341P8_DATA1_PIN,
            MODDEF_ILI9341P8_DATA2_PIN,
            MODDEF_ILI9341P8_DATA3_PIN,
            MODDEF_ILI9341P8_DATA4_PIN,
            MODDEF_ILI9341P8_DATA5_PIN,
            MODDEF_ILI9341P8_DATA6_PIN,
            MODDEF_ILI9341P8_DATA7_PIN,
        },
        .bus_width = 8,
        .max_transfer_bytes = 65536
    };

    err = esp_lcd_new_i80_bus(&bus_config, &sd->i80_bus_handle);
    if (err)
    	xsUnknownError("esp_lcd_new_i80_bus failed");

    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = MODDEF_ILI9341P8_CS_PIN,
        .pclk_hz = MODDEF_ILI9341P8_HZ,
        .trans_queue_depth = MODDEF_ILI9341P8_OPQUEUE,
        .on_color_trans_done = colorDone,
        .user_ctx = sd,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .flags = {
			.cs_active_high = 0,
			.pclk_active_neg = 0,
			.pclk_idle_low = 0,
			.reverse_color_bits = 0,
#if kCommodettoBitmapFormat == kCommodettoBitmapRGB565LE
			.swap_color_bytes = 1
#endif
        },
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    err = esp_lcd_new_panel_io_i80(sd->i80_bus_handle, &io_config, &sd->io_handle);
    if (err)
    	xsUnknownError("esp_lcd_new_panel_io_i80 failed");

	sd->dispatch = (PixelsOutDispatch)&gPixelsOutDispatch;

#ifdef MODDEF_ILI9341P8_RST_PIN
	modGPIOInit(&sd->rst, NULL, MODDEF_ILI9341P8_RST_PIN, kModGPIOOutput);
	modGPIOWrite(&sd->rst, 1);
#endif

#ifdef MODDEF_ILI9341P8_READ_PIN
	modGPIOInit(&sd->readEn, NULL, MODDEF_ILI9341P8_READ_PIN, kModGPIOOutput);
	modGPIOWrite(&sd->readEn, 1);		// Read high (inactive)
#endif

#ifdef MODDEF_ILI9341P8_TEARINGEFFECT_PIN
//	modGPIOInit(&sd->tearingEffect, NULL, MODDEF_ILI9341P8_TEARINGEFFECT_PIN, kModGPIOInput);
	gpio_pad_select_gpio(MODDEF_ILI9341P8_TEARINGEFFECT_PIN);
	gpio_set_direction(MODDEF_ILI9341P8_TEARINGEFFECT_PIN, GPIO_MODE_INPUT);
	gpio_set_pull_mode(MODDEF_ILI9341P8_TEARINGEFFECT_PIN, GPIO_FLOATING);
	gpio_install_isr_service(0);
	gpio_set_intr_type(MODDEF_ILI9341P8_TEARINGEFFECT_PIN, GPIO_INTR_NEGEDGE);
	gpio_isr_handler_add(MODDEF_ILI9341P8_TEARINGEFFECT_PIN, tearingEffectISR, sd);
#endif

	sd->ops = xQueueCreate(MODDEF_ILI9341P8_OPQUEUE, sizeof(int));
	sd->opZero = 0;

	sd->colorsInFlight = xSemaphoreCreateCounting(2, 2);		// async client uses two pixel buffers

	ili9341Init(sd);

#ifdef MODDEF_ILI9341P8_BACKLIGHT_PIN
	modGPIOInit(&sd->backlight, NULL, MODDEF_ILI9341P8_BACKLIGHT_PIN, kModGPIOOutput);
	modGPIOWrite(&sd->backlight, MODDEF_ILI9341P8_BACKLIGHT_OFF);
#endif
}

void xs_ILI9341p8_begin(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	CommodettoCoordinate x = (CommodettoCoordinate)xsmcToInteger(xsArg(0));
	CommodettoCoordinate y = (CommodettoCoordinate)xsmcToInteger(xsArg(1));
	CommodettoDimension w = (CommodettoDimension)xsmcToInteger(xsArg(2));
	CommodettoDimension h = (CommodettoDimension)xsmcToInteger(xsArg(3));

	ili9341Begin(sd, x, y, w, h);
}

void xs_ILI9341p8_send(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
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

	ili9341Send((PocoPixel *)data, count, sd);
}

void xs_ILI9341p8_end(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	ili9341End(sd);
}

void xs_ILI9341p8_pixelsToBytes(xsMachine *the)
{
	int count = xsmcToInteger(xsArg(0));
	xsmcSetInteger(xsResult, ((count * kCommodettoPixelSize) + 7) >> 3);
}

void xs_ILI9341p8_get_pixelFormat(xsMachine *the)
{
	xsmcSetInteger(xsResult, kCommodettoBitmapFormat);
}

void xs_ILI9341p8_get_width(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, MODDEF_ILI9341P8_WIDTH);
}

void xs_ILI9341p8_get_height(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, MODDEF_ILI9341P8_HEIGHT);
}

void xs_ILI9341p8_get_c_dispatch(xsMachine *the)
{
	xsResult = xsThis;
}

void xs_ILI9341p8_command(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	uint8_t command = (uint8_t)xsmcToInteger(xsArg(0));
	uint16_t dataSize = 0;
	uint8_t *data = NULL;

	if (xsmcArgc > 1) {
		dataSize = (uint16_t)xsmcGetArrayBufferLength(xsArg(1));
		data = xsmcToArrayBuffer(xsArg(1));
	}

	ili9341Command(sd, command, data, dataSize);
}

void xs_ILI9341p8_close(xsMachine *the)
{
	spiDisplay sd = xsmcGetHostData(xsThis);
	if (!sd) return;
	xs_ILI9341p8_destructor(sd);
	xsmcSetHostData(xsThis, NULL);
}

void ili9341Send(PocoPixel *pixels, int byteLength, void *refcon)
{
	spiDisplay sd = refcon;
	uint8_t sync = byteLength > 0; 
	if (!sync)
		byteLength = -byteLength;

#ifdef MODDEF_ILI9341P8_TEARINGEFFECT_PIN
	if (sd->firstBuffer) {
		sd->firstBuffer = 0;
//		while (0 == modGPIORead(&sd->tearingEffect))
//			;

		sd->te_byteLength = byteLength;
		sd->te_pixels = pixels;

//	esp_lcd_panel_io_tx_color(sd->io_handle, 0x2C, pixels, byteLength);
	}
	else
 #endif
	{
		int one = 1;
		xQueueSend(sd->ops, &one, portMAX_DELAY);
		esp_lcd_panel_io_tx_color(sd->io_handle, 0x2C, pixels, byteLength);

		int lines = (byteLength >> 1) / sd->updateWidth;
		sd->yMin += lines;
		sd->updateLinesRemaining -= lines;

		if (sd->updateLinesRemaining) {
			// reversing endian!
			uint8_t *data = sd->data + (4 * (sd->ping++ & 7)); 
			data[0] = sd->yMin & 0xff;
			data[1] = sd->yMin >> 8;;
			data[2] = sd->yMax & 0xff;
			data[3] = sd->yMax >> 8;;
			ili9341CommandAsync(sd, 0x2b, data, 4);
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

// delay of 0 is end of commands
#define kDelayMS (255)

#if 0
	// HX8357
	static const uint8_t gInit[] ICACHE_RODATA_ATTR = {
			0x01, 0,
			0xB9, 3, 0xFF, 0x83, 0x57,
			kDelayMS, 250,
			0xB3, 4, 0x00, 0x00, 0x06, 0x06,
			0xB6, 1, 0x25,  // -1.52V
			0xB0, 1, 0x68,  // Normal mode 70Hz, Idle mode 55 Hz
			0xCC, 1, 0x05,  // BGR, Gate direction swapped
			0xB1, 6, 0x00, 0x15, 0x1C, 0x1C, 0x83, 0xAA,
			0xC0, 6, 0x50, 0x50, 0x01, 0x3C, 0x1E, 0x08,
			// MEME GAMMA HERE
			0xB4, 7, 0x02, 0x40, 0x00, 0x2A, 0x2A, 0x0D, 0x78,
			0x3A, 1, 0x55,
			0x36, 1, (0xC0 ^ (MODDEF_ILI9341P8_FLIPY ? 0x80 : 0)) ^ (MODDEF_ILI9341P8_FLIPX ? 0x40 : 0), \
			0x44, 2, 0x00, 0x00,		// HX8357 doens't seem to implement this
			0x35, 1, 0x00,
	//		0x11, 0,
	//		kDelayMS, 150,
	//		0x29, 0,
	//		kDelayMS, 50,

			kDelayMS, 0
	};
#else
	// ST7789V
	static const uint8_t gInit[] ICACHE_RODATA_ATTR = {
		// Rongstar: ---- display and color format setting ----
		0x11, 0,		// Output sleep
		kDelayMS, 200,
		0x36, 1, 0,		// MY,MV,MX,RGB
		0x3a, 1, 0x05,	// Pixel Format
		0x21, 1, 1,		// pixel invert

		// Rongstar: ---- ST7789V Frame rate setting ----
		0xb2, 5, 0x0c, 0x0c, 0x00, 0x33, 0x33,
		0xb7, 1, 0x35, 

		// Rongstar: ---- ST7789V Power setting ----
		0xbb, 1, 0x35,
		0xc0, 1, 0x2c,
		0xc2, 1, 0x01,
		0xc3, 1, 0x0b,
		0xc4, 1, 0x20,
		0xc6, 1, 0x0f,
		0xd0, 2, 0xa4, 0xa1,

		// Rongstar: ---- ST7789V Gamma setting ----
		0xe0, 14, 0xd0, 0x00, 0x02, 0x07,  0x0b, 0x1a, 0x31, 0x54,  0x40, 0x29, 0x12, 0x12,  0x12, 0x17,
		0xe1, 14, 0xd0, 0x00, 0x02, 0x07,  0x05, 0x25, 0x2d, 0x44,  0x45, 0x1c, 0x18, 0x16,  0x1c, 0x1d,
		kDelayMS, 80,

		kDelayMS, 0
	};
#endif

void ili9341Init(spiDisplay sd)
{
	const uint8_t *cmds;

#ifdef MODDEF_ILI9341P8_RST_PIN
	modGPIOWrite(&sd->rst, 0);
	modDelayMilliseconds(10);
	modGPIOWrite(&sd->rst, 1);
	modDelayMilliseconds(1);
#endif

	cmds = gInit;
	while (true) {
		uint8_t cmd = c_read8(cmds++);
		if (kDelayMS == cmd) {
			uint8_t ms = c_read8(cmds++);
			if (0 == ms)
				break;
			modDelayMilliseconds(ms);
		}
		else {
			if (0x36 == cmd)
				sd->memoryAccessControl = c_read8(cmds + 1);
			uint8_t count = c_read8(cmds++);
			ili9341Command(sd, cmd, cmds, count);
			cmds += count;
		}
	}

	sd->firstFrame = true;
}

void ili9341Begin(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h)
{
	spiDisplay sd = refcon;
	uint16_t xMin, xMax, yMin, yMax;

	xMin = x + MODDEF_ILI9341P8_COLUMN_OFFSET;
	yMin = y + MODDEF_ILI9341P8_ROW_OFFSET;
	
	xMax = xMin + w - 1;
	yMax = yMin + h - 1;

	sd->updateWidth = w;
	sd->updateLinesRemaining = h;
	sd->yMin = yMin;
	sd->yMax = yMax;
	sd->firstBuffer = !sd->firstFrame && !sd->isContinue;

	uint8_t *data = sd->data + (4 * (sd->ping++ & 7)); 
	data[0] = xMin & 0xff;
	data[1] = xMin >> 8;
	data[2] = xMax & 0xff;
	data[3] = xMax >> 8;
	ili9341CommandAsync(sd, 0x2a, data, 4);

	data = sd->data + (4 * (sd->ping++ & 7)); 
	data[0] = yMin & 0xff;
	data[1] = yMin >> 8;;
	data[2] = yMax & 0xff;
	data[3] = yMax >> 8;
	ili9341CommandAsync(sd, 0x2b, data, 4);

	xSemaphoreTake(sd->colorsInFlight, portMAX_DELAY);
}

void ili9341Continue(void *refcon)
{
	spiDisplay sd = refcon;
	sd->isContinue = true;
}

void ili9341End(void *refcon)
{
	spiDisplay sd = refcon;

	sd->isContinue = false;
	if (sd->firstFrame) {
		sd->firstFrame = false;

		ili9341CommandAsync(sd, 0x11, NULL, 0);
		ili9341CommandAsync(sd, 0x29, NULL, 0);

#ifdef MODDEF_ILI9341P8_BACKLIGHT_PIN
		modGPIOWrite(&sd->backlight, MODDEF_ILI9341P8_BACKLIGHT_ON);
#endif
	}
}

#ifdef MODDEF_ILI9341P8_TEARINGEFFECT_PIN

void tearingEffectISR(void *refcon)
{
	spiDisplay sd = refcon;

	if (sd->te_pixels) {
		esp_lcd_panel_io_tx_color(sd->io_handle, 0x2C, sd->te_pixels, sd->te_byteLength);		//@@ ISR safe!?!?!?

		sd->yMin += (sd->te_byteLength >> 1) / sd->updateWidth;  

		// reversing endian!
		uint8_t *data = sd->data + (4 * (sd->ping++ & 7)); 
		data[0] = sd->yMin & 0xff;
		data[1] = sd->yMin >> 8;;
		data[2] = sd->yMax & 0xff;
		data[3] = sd->yMax >> 8;;
		ili9341CommandAsync(sd, 0x2b, data, 4);		//@@ not ISR safe!

		sd->te_pixels = NULL;
	}
}

#endif
