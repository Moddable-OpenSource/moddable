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

#include "xsesp.h"

#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>

#include "modGPIO.h"

static const uint32_t gPixMuxAddr[] ICACHE_RODATA_ATTR = {
	PERIPHS_IO_MUX_GPIO0_U,
	PERIPHS_IO_MUX_U0TXD_U,
	PERIPHS_IO_MUX_GPIO2_U,
	PERIPHS_IO_MUX_U0RXD_U,
	PERIPHS_IO_MUX_GPIO4_U,
	PERIPHS_IO_MUX_GPIO5_U,
	PERIPHS_IO_MUX_SD_CLK_U,
	PERIPHS_IO_MUX_SD_DATA0_U,
	PERIPHS_IO_MUX_SD_DATA1_U,
	PERIPHS_IO_MUX_SD_DATA2_U,
	PERIPHS_IO_MUX_SD_DATA3_U,
	PERIPHS_IO_MUX_SD_CMD_U,
	PERIPHS_IO_MUX_MTDI_U,
	PERIPHS_IO_MUX_MTCK_U,
	PERIPHS_IO_MUX_MTMS_U,
	PERIPHS_IO_MUX_MTDO_U
};

static const uint8_t gPixMuxValue[] ICACHE_RODATA_ATTR = {
	FUNC_GPIO0,
	FUNC_GPIO1,
	FUNC_GPIO2,
	FUNC_GPIO3,
	FUNC_GPIO4,
	FUNC_GPIO5,
	FUNC_GPIO6,
	FUNC_GPIO7,
	FUNC_GPIO8,
	FUNC_GPIO9,
	FUNC_GPIO10,
	FUNC_GPIO11,
	FUNC_GPIO12,
	FUNC_GPIO13,
	FUNC_GPIO14,
	FUNC_GPIO15
};

/*
	gpio
*/

//@@ what else needs to be initialized here?
#define GPCD   2  // DRIVER 0: normal, 1: open drain

#define GPIO_INIT_OUTPUT(index, opendrain) \
		*(volatile uint32_t *)(PERIPHS_GPIO_BASEADDR + 0x10) |= (1 << index);					/* enable for write */ \
		*(volatile uint32_t *)(PERIPHS_GPIO_BASEADDR + 0x28 + (index << 2)) &= ~((opendrain ? 0 : 1) << GPCD);	/* normal (not open-drain) */ \

//@@ test THIS!!
#define GPIO_INIT_INPUT(index) \
		*(volatile uint32_t *)(PERIPHS_GPIO_BASEADDR + 0x10) &= ~(1 << index);					/* disable write (e.g. read) */ \
		*(volatile uint32_t *)(PERIPHS_GPIO_BASEADDR + 0x28 + (index << 2)) &= ~(1 << GPCD);	/* normal (not open-drain) */

#define GPIO_CLEAR(index) (GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1 << index))
#define GPIO_SET(index) (GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1 << index))

#define kUninitializedPin (255)

int modGPIOInit(modGPIOConfiguration config, const char *port, uint8_t pin, uint32_t mode)
{
	int result;

	if ((pin > 16) || port) {
		config->pin = kUninitializedPin;
		return -1;
	}

	config->pin = pin;

	result = modGPIOSetMode(config, mode);
	if (result) {
		config->pin = kUninitializedPin;
		return result;
	}

	return 0;
}

void modGPIOUninit(modGPIOConfiguration config)
{
	config->pin = kUninitializedPin;
}

int modGPIOSetMode(modGPIOConfiguration config, uint32_t mode)
{
	switch (mode) {
		case kModGPIOInput:
		case kModGPIOInputPullUp:
		case kModGPIOInputPullDown:
			if (config->pin < 16) {
				PIN_FUNC_SELECT(gPixMuxAddr[config->pin], c_read8(&gPixMuxValue[config->pin]));
				GPIO_INIT_INPUT(config->pin);

				if (mode == kModGPIOInputPullUp)
					*(volatile uint32_t *)gPixMuxAddr[config->pin] |= 1 << 7;
				else if (mode == kModGPIOInputPullDown)
					*(volatile uint32_t *)gPixMuxAddr[config->pin] |= 1 << 6;
			}
			else {
				if (kModGPIOInput != mode)
					return -1;

				WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
							   (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1); 	// mux configuration for XPD_DCDC and rtc_gpio0 connection

				WRITE_PERI_REG(RTC_GPIO_CONF,
							   (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);	//mux configuration for out enable

				WRITE_PERI_REG(RTC_GPIO_ENABLE,
							   READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe);	//out disable
			}
			break;

		case kModGPIOOutput:
		case kModGPIOOutputOpenDrain:
			if (config->pin < 16) {
				PIN_FUNC_SELECT(gPixMuxAddr[config->pin], c_read8(&gPixMuxValue[config->pin]));
				GPIO_INIT_OUTPUT(config->pin, kModGPIOOutputOpenDrain == mode);
				GPIO_CLEAR(config->pin);
			}
			else {
				if (kModGPIOOutputOpenDrain == mode)
					return -1;

				WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
							   (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1); 	// mux configuration for XPD_DCDC to output rtc_gpio0

				WRITE_PERI_REG(RTC_GPIO_CONF,
							   (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);	//mux configuration for out enable

				WRITE_PERI_REG(RTC_GPIO_ENABLE,
							   (READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe) | (uint32)0x1);	//out enable
			}
			break;

		default:
			return -1;
	}

	return 0;
}

uint8_t modGPIORead(modGPIOConfiguration config)
{
	if (config->pin < 16)
		return (GPIO_REG_READ(GPIO_IN_ADDRESS) >> config->pin) & 1;

	if (16 == config->pin)
		return READ_PERI_REG(RTC_GPIO_IN_DATA) & 1;

	return kModGPIOReadError;
}

void modGPIOWrite(modGPIOConfiguration config, uint8_t value)
{
	if (config->pin < 16) {
		if (value)
			GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1 << config->pin);
		else
			GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1 << config->pin);
	}
	else if (16 == config->pin) {
		WRITE_PERI_REG(RTC_GPIO_OUT,
					   (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(value & 1));
	}
}
