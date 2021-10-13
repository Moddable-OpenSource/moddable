/*
* Copyright (c) 2021  Moddable Tech, Inc.
*
*   This file is part of the Moddable SDK.
*
*   This work is licensed under the
*       Creative Commons Attribution 4.0 International License.
*   To view a copy of this license, visit
*       <http://creativecommons.org/licenses/by/4.0>.
*   or send a letter to Creative Commons, PO Box 1866,
*   Mountain View, CA 94042, USA.
*
*/

const Digital = device.io.Digital;
const SPI = device.io.SPI;

if (undefined !== device.pin.backlight) {
	const backlight = new Digital({
		pin: device.pin.backlight,
		mode: Digital.Output,
	});
	backlight.write(0);
}

const dc = new Digital({
	pin: device.pin.displayDC,
	mode: Digital.Output,
});

const spi = new SPI({
	...device.SPI.default,
	hz: 40_000_000,
	select: device.pin.displaySelect,
	active: 0
});

// registers for display in Moddable One and Moddable Two
doCmd(0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02);
doCmd(0xCF, 0x00, 0xC1, 0X30);
doCmd(0xE8, 0x85, 0x00, 0x78);
doCmd(0xEA, 0x00, 0x00);
doCmd(0xED, 0x64, 0x03, 0x12, 0x81);
doCmd(0xF7, 0x20);
doCmd(0xC0, 0x23);
doCmd(0xC1, 0x10);
doCmd(0xC5, 0x3e, 0x28);
doCmd(0xC7, 0x86);
doCmd(0x3A, 0x55);
doCmd(0xB1, 0x00, 0x18);
doCmd(0xB6, 0x08, 0x82, 0x27);
doCmd(0xF2, 0x00);
doCmd(0x26, 0x01);

if (0) {
	doCmd(0xE0, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00);
	doCmd(0xE1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F);

	doCmd(0x36, 0x48);		// moddable zero
}
else {
	doCmd(0xE0, 0xD0, 0x08, 0x11, 0x08, 0x0C, 0x15, 0x39, 0x33, 0x50, 0x36, 0x13, 0x14, 0x29, 0x2d);
	doCmd(0xE1, 0xD0, 0x08, 0x11, 0x08, 0x06, 0x06, 0x39, 0x44, 0x51, 0x0b, 0x16, 0x14, 0x2f, 0x31);

	doCmd(0x36, 0xF0);	// moddable one, moddable two
	doCmd(0x21);
}

doCmd(0x11);
doCmd(0x29);

fill(0xFFFF, 0, 240, 0, 320);
fill(0xF800, 0, 240, 0, 320);
fill(0x001F, 0, 240, 0, 320);
fill(0x07E0, 0, 240, 0, 320);

fill(0xF81F, 0, 120, 0, 160);
fill(0x07FF, 120, 240, 160, 320);

function fill(color, xMin, xMax, yMin, yMax) {
	doCmd(0x2a, xMin >> 8, xMin & 0xff, xMax >> 8, xMax & 0xff);
	doCmd(0x2b, yMin >> 8, yMin & 0xff, yMax >> 8, yMax & 0xff);
	doCmd(0x2c);

	dc.write(1);

	const lines = 4;
	const pixels = new Uint16Array((xMax - xMin) * lines);
	pixels.fill((color >> 8) | (color << 8));		// big endian

	for (yMax -= yMin; yMax > 0; yMax -= lines)
		spi.write(pixels.buffer);
}

function doCmd(register, ...data) {
	spi.flush(true);
	dc.write(0);
	spi.write(Uint8Array.of(register));

	if (data.length) {
		dc.write(1);
		spi.write(Uint8Array.from(data));
	}
}
