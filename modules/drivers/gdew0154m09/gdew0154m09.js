/*
 * Copyright (c) 2021  Moddable Tech, Inc.
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

	Data sheet for controller: https://v4.cecdn.yun300.cn/100001_1909185148/GDEW0154M09-200709.pdf
	Reference implementation: https://github.com/m5stack/M5-CoreInk/blob/master/src/utility/Ink_eSPI.cpp
*/

import Timer from "timer";
import Bitmap from "commodetto/Bitmap"
import Dither from "commodetto/Dither"

const WFT0154CZB3_LIST = Uint8Array.of(
    11,                     // (11 commands in list)
    0x00,2,0xdf,0x0e,       // panel setting
    0x4D,1,0x55,            // FITIinternal code
    0xaa,1,0x0f,
    0xe9,1,0x02,
    0xb6,1,0x11,
    0xf3,1,0x0a,
    0x61,3,0xc8,0x00,0xc8,  // resolution setting
    0x60,1,0x00,            // Tcon setting
    0x50,1,0xd7,
    0xe3,1,0x00,
    0x04,0                  // power on
);

const lut_vcomDC1 = Uint8Array.of(  
    0x01, 0x04, 0x04, 0x03, 0x01, 0x01, 0x01, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                
const lut_ww1 = Uint8Array.of(  
    0x01, 0x04, 0x04, 0x03, 0x01, 0x01, 0x01, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                
const lut_bw1 = Uint8Array.of(  
    0x01, 0x84, 0x84, 0x83, 0x01, 0x01, 0x01, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                
const lut_wb1 = Uint8Array.of(  
    0x01, 0x44, 0x44, 0x43, 0x01, 0x01, 0x01, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

const lut_bb1 = Uint8Array.of(  
    0x01, 0x04, 0x04, 0x03, 0x01, 0x01, 0x01, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

const INK_FULL_MODE =       0x00;
const INK_PARTIAL_MODE =    0x01;

class EPD {
	mode = INK_FULL_MODE;

	constructor() {
		const Digital = device.io.Digital;
		const SPI = device.io.SPI;

		this.select = new Digital({
			pin: device.pin.epdSelect,
			mode: Digital.Output,
		});
		this.select.write(1);

		this.dc = new Digital({
			pin: device.pin.epdDC,
			mode: Digital.Output,
		});
		this.dc.write(1);

		this.reset = new Digital({
			pin: device.pin.epdReset,
			mode: Digital.Output,
		});
		this.reset.write(1);

		this.busy = new Digital({
			pin: device.pin.epdBusy,
			mode: Digital.Input,
		});

		this.spi = new SPI({
			...device.SPI.default,
			hz: 10_000_000,
			mode: 3
		});
		this.spi.buffer8 = new Uint8Array(1);
		this.spi.write8 = function(value) {
			const buffer8 = this.buffer8;
			buffer8[0] = value;
			this.write(buffer8);
		}; 

		this.reset.write(1);
		Timer.delay(10);
		this.reset.write(0);
		Timer.delay(100);
		this.reset.write(1);
		Timer.delay(100);

		this.waitBusy(1000);

		const list = WFT0154CZB3_LIST;
		let length = list[0], offset = 1;
		while (length--) {
			this.writeCMD(list[offset]);

			const data = list[offset + 1];
			for (let i = 0; i < data; i++)
				this.writeData(list[offset + 2 + i]);

			offset += 2 + data;
		}

		Timer.delay(100);
		this.waitBusy(1000);
	}
	close() {
		this.select?.close();
		this.dc?.close();
		this.reset?.close();
		this.busy?.close();
		this.spi?.close();
	
		delete this.select;
		delete this.dc;
		delete this.reset;
		delete this.busy;
		delete this.spi;
	}
	writeCMD(cmd) {
		this.select.write(0);
		this.dc.write(0);
		this.spi.write8(cmd);
		this.select.write(1);
	}
	writeData(data) {
		this.select.write(0);
		this.dc.write(1);
		this.spi.write8(data);
		this.select.write(1);
	}
	writeDataBuffer(data) {
		this.select.write(0);
		this.dc.write(1);
		this.spi.write(data);
		this.select.write(1);
	}
	isBusy() {
		return !this.busy.read();
	}
	waitBusy(wait) {
		wait += Date.now();
		do {
			if (!this.isBusy())
				return;
		} while (wait > Date.now());
		
		throw new Error("time out");
	}
	setArea(x, y, w, h) {
		if ((x | w) & 3)
			throw new Error;
	
		this.writeCMD(0x90);   //resolution setting
		this.writeData (x);   //x-start     
		this.writeData (x + w - 1);   //x-end  
		this.writeData (0);   //x Reserved 

		this.writeData (y);   //y-start    
		this.writeData (0);   //y Reserved   
		this.writeData (y + h);   //y-end  
		this.writeData (0x01); 
	}
	switchMode(mode) {
		if (mode === this.mode)
			return;

		if (INK_PARTIAL_MODE === mode) {
			this.writeCMD(0x91);   // enter partial mode

			this.writeCMD(0x00);	//panel setting
			this.writeData(0xff);
			this.writeData(0x0e);

			this.writeCMD(0x20);
			this.writeDataBuffer(lut_vcomDC1);

			this.writeCMD(0x21);
			this.writeDataBuffer(lut_ww1);
		
			this.writeCMD(0x22);
			this.writeDataBuffer(lut_bw1);
		
			this.writeCMD(0x23);
			this.writeDataBuffer(lut_wb1);

			this.writeCMD(0x24);
			this.writeDataBuffer(lut_bb1);
		}
		else {
			this.writeCMD(0x92);   // exit partial mode

			this.writeCMD(0x00);	//panel setting
			this.writeData(0xdf);
			this.writeData(0x0e);
		}

		this.mode = mode;
	}
}

class Display {		// implementation of PixelsOut
	#epd = new EPD;
	#buffer = new Uint8Array((200 * 200) >> 3);
	#dither = new Dither({width: 200}); 

	constructor(options) {		
		this.#epd.refresh = true;
	}
	close() {
		if (this.#epd) {
			const epd = this.#epd;

			epd.writeCMD(0x50);
			epd.writeData(0xF7);
			epd.writeCMD(0x02);		// power off
			epd.waitBusy(5_000);
			epd.writeCMD(0x07);		// deep sleep
			epd.writeData(0xA5);

			epd.close();
			this.#epd = undefined;
		}

		this.#dither?.close();
		this.#dither = undefined;
		this.#buffer = undefined;
	}
	configure(options) {
		const {refresh, previous, dither} = options;

		if (undefined !== refresh)
			this.#epd.refresh = refresh;
		
		if (undefined !== previous)
			this.#epd.previous = previous;

		if (undefined !== dither) {
			this.#dither.close();
			
			let algorithm = dither;
			if (false === algorithm)
				algorithm = "none";
			else if (true === algorithm)
				algorithm = undefined	// default
			this.#dither = new Dither({width: 200, algorithm});
		}
	}
	begin(x, y, width, height) {
		const epd = this.#epd;

		if (epd.refresh) {
			epd.switchMode(INK_FULL_MODE);
			this.refresh();
			delete epd.refresh;
		}
		epd.switchMode(INK_PARTIAL_MODE);

		epd.setArea(x, y, width, height);

		if (!epd.previous)
			this.#output(0x10);

		this.#buffer.position = 0;
		this.#dither.reset();
	}
	send(data, offset, byteLength) {
		const buffer = this.#buffer;
		this.#dither.send(Math.idiv(byteLength, 200), data, offset, buffer, buffer.position);
		buffer.position += byteLength >> 3;
	}
	end() {
		const epd = this.#epd;

		if (!epd.previous) {
			this.#output(0x13);
			epd.writeCMD(0x12);
			epd.waitBusy(1000);
		}
		delete epd.previous;
	}
	continue() {
		return this.end();
	}
	adaptInvalid(area) {
		area.x = 0;
		area.y = 0;
		area.w = 200;
		area.h = 200;
	}
	get width() {
		return 200;
	}
	get height() {
		return 200;
	}
	get pixelFormat() {
		return Bitmap.Gray256;
	}
	pixelsToBytes(pixels) {
		return pixels;
	}
	refresh() {
		const epd = this.#epd;

		this.#buffer.fill(0xFF);		// all white
		this.#output(0x10);				// to background
        Timer.delay(2);
		this.#buffer.fill(0);			// all black
		this.#output(0x13);				// to forground
        Timer.delay(2);
        epd.writeCMD(0x12);				// display
        epd.waitBusy(1000);

		this.#buffer.fill(0);			// all black
		this.#output(0x10);				// to background
        Timer.delay(2);
		this.#buffer.fill(0xFF);		// all white
		this.#output(0x13);				// to foreground
        Timer.delay(2);
        epd.writeCMD(0x12);				// display
        epd.waitBusy(1000);
	}
	#output(cmd) {
		const epd = this.#epd;
		epd.writeCMD(cmd);
		epd.select.write(0);
		epd.dc.write(1);
		epd.spi.write(this.#buffer);
		epd.select.write(1);
	}
}

export default Display;
