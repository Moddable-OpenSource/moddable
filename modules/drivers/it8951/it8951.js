/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
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

import Timer from "timer";
import Bitmap from "commodetto/Bitmap";

/*
	EPD driver reference: https://github.com/m5stack/M5EPD/blob/63f6eb34697b0120e68d279fe0e22e5ec3aba61b/src/M5EPD_Driver.cpp
 */

const IT8951_TCON_SYS_RUN =         0x0001;
const IT8951_TCON_STANDBY =         0x0002;
const IT8951_TCON_SLEEP =           0x0003;
const IT8951_TCON_REG_RD =          0x0010;
const IT8951_TCON_REG_WR =          0x0011;

const IT8951_TCON_MEM_BST_RD_T =    0x0012;
const IT8951_TCON_MEM_BST_RD_S =    0x0013;
const IT8951_TCON_MEM_BST_WR =      0x0014;
const IT8951_TCON_MEM_BST_END =     0x0015;

const IT8951_TCON_LD_IMG =          0x0020;
const IT8951_TCON_LD_IMG_AREA =     0x0021;
const IT8951_TCON_LD_IMG_END =      0x0022;

//I80 User defined command code
const IT8951_I80_CMD_DPY_AREA =     0x0034;
const IT8951_I80_CMD_GET_DEV_INFO = 0x0302;
const IT8951_I80_CMD_DPY_BUF_AREA = 0x0037;
const IT8951_I80_CMD_VCOM =         0x0039;

//Endian Type
const IT8951_LDIMG_L_ENDIAN =    0;
const IT8951_LDIMG_B_ENDIAN =    1;

//Pixel mode (Bit per Pixel)
const IT8951_2BPP =             0;
const IT8951_3BPP =             1;
const IT8951_4BPP =             2;
const IT8951_8BPP =             3;

const M5EPD_PANEL_W =   960;
const M5EPD_PANEL_H =   540;

const IT8951_DISPLAY_REG_BASE =     0x1000; //Register RW access

const IT8951_UP0SR =       (IT8951_DISPLAY_REG_BASE + 0x134); //Update Parameter0 Setting Reg
const IT8951_UP1SR =       (IT8951_DISPLAY_REG_BASE + 0x138); //Update Parameter1 Setting Reg
const IT8951_LUT0ABFRV =   (IT8951_DISPLAY_REG_BASE + 0x13C); //LUT0 Alpha blend and Fill rectangle Value
const IT8951_UPBBADDR =    (IT8951_DISPLAY_REG_BASE + 0x17C); //Update Buffer Base Address
const IT8951_LUT0IMXY =    (IT8951_DISPLAY_REG_BASE + 0x180); //LUT0 Image buffer X/Y offset Reg
const IT8951_LUTAFSR =     (IT8951_DISPLAY_REG_BASE + 0x224); //LUT Status Reg (status of All LUT Engines)
const IT8951_BGVR =        (IT8951_DISPLAY_REG_BASE + 0x250); //Bitmap (1bpp) image color table

const IT8951_SYS_REG_BASE =         0x0000;

//Address of System Registers
const IT8951_I80CPCR =              (IT8951_SYS_REG_BASE + 0x04);

//Memory Converter Registers
const IT8951_MCSR_BASE_ADDR =       0x0200;
const IT8951_MCSR =                 (IT8951_MCSR_BASE_ADDR + 0x0000);
const IT8951_LISAR =                (IT8951_MCSR_BASE_ADDR + 0x0008);

const UpdateMode = {
				  //   Ghosting  Update Time  Usage
    INIT:     0,  // * N/A       2000ms       Display initialization, 
    DU:       1,  //   Low       260ms        Monochrome menu, text input, and touch screen input 
    GC16:     2,  // * Very Low  450ms        High quality images
    GL16:     3,  // * Medium    450ms        Text with white background 
    GLR16:    4,  //   Low       450ms        Text with white background
    GLD16:    5,  //   Low       450ms        Text and graphics with white background 
    DU4:      6,  // * Medium    120ms        Fast page flipping at reduced contrast
    A2:       7,  //   Medium    290ms        Anti-aliased text in menus / touch and screen input 
    NONE:     8
};        // The ones marked with * are more commonly used
Object.freeze(UpdateMode, true);

class EPD {
	_tar_memaddr = 0x001236E0;
	_pix_bpp;
	_endian_type = IT8951_LDIMG_B_ENDIAN;
	_pix_bpp = IT8951_4BPP;
	_rotation = 0;
	_updateArea = new Uint16Array(7);
	_setArea = new Uint16Array(this._updateArea.buffer, 0, 5);

	constructor() {
		const Digital = device.io.Digital;
		const SPI = device.io.SPI;

		this.select = new Digital({
			pin: device.pin.epdSelect,
			mode: Digital.Output,
		});
		this.select.write(1);

		this.busy = new Digital({
			pin: device.pin.epdBusy,
			mode: Digital.Input,
		});

		this.spi = new SPI({
			...device.SPI.default,
			hz: 10_000_000
		});
		this.spi.buffer16 = new Uint8Array(2);
		this.spi.write16 = function(value) {
			const buffer = this.buffer16;
			buffer[0] = value >> 8;
			buffer[1] = value;
			this.write(buffer);
		}; 
		this.spi.transfer16 = function(value) {
			const buffer = this.buffer16;
			buffer[0] = value >> 8;
			buffer[1] = value;
			this.transfer(buffer);
			return (buffer[0] << 8) | buffer[1];
		}; 

		this.getSysInfo();

		this.writeCommand(IT8951_TCON_SYS_RUN);
		this.writeRegister(IT8951_I80CPCR, 0x0001); // enable pack write

		//set vcom to -2.30v
		this.writeCommand(IT8951_I80_CMD_VCOM);
		this.writeWord(0x0001);
		this.writeWord(2300);

		Timer.delay(1000);
	}
	close() {
		this.spi?.close();
		this.select?.close();
		this.busy?.close();

		delete this.spi;
		delete this.select;
		delete this.busy;
	}
	clear() {
		this.updateFull(UpdateMode.INIT);
	}
	updateFull(mode) {
        this.updateArea(0, 0, M5EPD_PANEL_W, M5EPD_PANEL_H, mode);
	}
	checkAFSR() {
		const start = Date.now();
		const info = new Array(1);
		do { 
			this.writeCommand(IT8951_TCON_REG_RD);
			this.writeWord(IT8951_LUTAFSR);
			this.readWords(info);
			if (0 === info[0])
				return;

			if ((Date.now() - start) > 3000)
				throw new Error("time out");
		} while (true);
	}
	fillArea(x, y, w, h, color) {
		if (w & 3)
			throw new Error;

		if ((color < 0) || (color > 15))
			throw new Error;
	
		this.setTargetMemoryAddress(this._tar_memaddr);
        this.setArea(x, y, w, h);

		const pixels = new Uint16Array(w >> 2);
		pixels.fill(color | (color << 4) | (color << 8) | (color << 12));
		const buffer = pixels.buffer, spi = this.spi;

		this.select.write(0);
		spi.write(Uint16Array.of(0))
		while (h--)
			spi.write(buffer);
		this.select.write(1);

		this.writeCommand(IT8951_TCON_LD_IMG_END);
	}
	updateArea(x, y, w, h, mode) {
		if (UpdateMode.NONE === mode)
			return;

		this.checkAFSR();

		const area = this._updateArea;
		const rotation = this._rotation;
        if (!rotation) {
			area[0] = x;
			area[1] = y;
			area[2] = w;
			area[3] = h;
		}
		else if (1 === rotation) {
			area[0] = y;
			area[1] = M5EPD_PANEL_H - w - x;
			area[2] = h;
			area[3] = w;
		}
		else if (2 === rotation) {
			area[0] = M5EPD_PANEL_W - w - x;
			area[1] = M5EPD_PANEL_H - h - y;
			area[2] = w;
			area[3] = h;
		}
		else {
			area[0] = M5EPD_PANEL_W - h - y;
			area[1] = x;
			area[2] = h;
			area[3] = w;
		}
		area[4] = mode;
		area[5] = this._tar_memaddr;
		area[6] = this._tar_memaddr >> 16;

		this.writeArgs(IT8951_I80_CMD_DPY_BUF_AREA, area);
	}	
	
	setArea(x, y, w, h) {
		const area = this._setArea;

		area[0] = (this._endian_type << 8) | (this._pix_bpp << 4) | this._rotation;
		area[1] = x; 
		area[2] = y; 
		area[3] = w; 
		area[4] = h; 
		this.writeArgs(IT8951_TCON_LD_IMG_AREA, area);
	}
	writeArgs(command, buffer) {
		this.writeCommand(command);

		this.waitBusy();
		this.select.write(0);
		this.spi.write16(0x0000);
		this.waitBusy();

		for (let i = 0, length = buffer.length; i < length; i++) {
			this.spi.write16(buffer[i]);
			this.waitBusy();
		}
		this.select.write(1);
	}
	writeCommand(command) {
		this.waitBusy();
		this.select.write(0);
		this.spi.write16(0x6000);
		this.waitBusy();
		this.spi.write16(command);
		this.select.write(1);
	}
	setTargetMemoryAddress(address) {
		const h = (address >> 16) & 0xFFFF;
		const l = address & 0xFFFF;

    	this.writeRegister(IT8951_LISAR + 2, h);
    	this.writeRegister(IT8951_LISAR, l);
	}
	writeRegister(register, value) {
		this.writeCommand(IT8951_TCON_REG_WR);

		this.waitBusy();
		this.select.write(0);
		this.spi.write16(0x0000);
		this.waitBusy();
		this.spi.write16(register);
		this.waitBusy();
		this.spi.write16(value);
		this.select.write(1);
	}
	getSysInfo() {
    	this.writeCommand(IT8951_I80_CMD_GET_DEV_INFO);
		const info = this.readWords(new Array(20));
		const _tar_memaddr = (info[3] << 16) | info[2];
		if (_tar_memaddr !== this._tar_memaddr)
			trace(`unexpected _tar_memaddr value 0x${_tar_memaddr.toString(16)}\n`);
		
		if ((M5EPD_PANEL_W !== info[0]) || (M5EPD_PANEL_H !== info[1]))
			trace("unexpected panel dimensions\n");
	}
	writeWord(value) {
		this.waitBusy();
		this.select.write(0);
		this.spi.write16(0x0000);
		this.waitBusy();
		this.spi.write16(value);
		this.select.write(1);
	}
	waitBusy(timeout) {
		if (this.busy.read())
			return;

		if (!timeout)
			timeout = 3000;

		const start = Date.now();
		while (!this.busy.read()) {
			if ((Date.now() - start) > timeout)
				throw new Error("time out");
		}
	}
	readWords(result) {
    	this.waitBusy();
    	this.select.write(0);
    	this.spi.write16(0x1000);
    	this.waitBusy();

		// dummy
		this.spi.transfer16(0);
    	this.waitBusy();

		for (let i = 0, length = result.length; i < length; i++)
			result[i] = this.spi.transfer16(0);

		this.select.write(1);
		
		return result;
	}
}

const Filter = {
	none: undefined,
	negative: Uint8Array.of(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0),
	posterize: Uint8Array.of(0, 0, 0, 0, 3, 3, 3, 3, 7, 7, 7, 7, 15, 15, 15, 15),
	monochrome: Uint8Array.of(0, 0, 0, 0, 0, 0, 0, 0, 15, 15, 15, 15, 15, 15, 15, 15),
	bright: Uint8Array.of(0, 2, 3, 4, 5, 7, 8, 9, 11, 12, 13, 14, 15, 15, 15, 15),
	contrast: Uint8Array.of(0, 0, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 15)
};
Object.freeze(Filter, true);

class Display {		// implementation of PixelsOut
	#epd = new EPD;
	#area = {};
	#updateMode = UpdateMode.GLD16;
	#filter = Filter.none;
	#buffer = new Uint8Array(1024);

	constructor(options) {
		this.#epd.setTargetMemoryAddress(this.#epd._tar_memaddr);
	}
	close() {
		this.#epd?.close();
		this.#epd = undefined;
	}
	configure(options) {
		if (options.updateMode)
			 this.#updateMode = UpdateMode[options.updateMode] ?? UpdateMode.GLD16;

		if (options.filter)
			 this.#filter = ("string" === typeof options.filter) ? Filter[options.filter] : options.filter;
	}
	begin(x, y, width, height) {
		const epd = this.#epd, area = this.#area;

		if ((x | width) & 3)
			throw new Error;

		if (this.#epd._rotation & 1) { 
			if ((y | height) & 3)
				throw new Error;
		}

		if (area.continue) {
			if (x < area.x) {
				area.width += area.x - x;
				area.x = x;
			}
			if ((x + width) > (area.x + area.width))
				area.width = (x + width) - area.x;

			if (y < area.y) {
				area.height += area.y - y;
				area.y = y;
			}
			if ((y + height) > (area.y + area.height))
				area.height = (y + height) - area.y;
		}
		else
			area.x = x, area.y = y, area.width = width, area.height = height;

        epd.setArea(x, y, width, height);

		epd.select.write(0);
		epd.spi.write16(0);

		this.#buffer.position = 0;
	}
	send(src, offset, byteLength) {
		const filter = this.#filter, buffer = this.#buffer;
		let position = buffer.position;

		while (byteLength) {
			let use = byteLength;
			if (use > 1024 - position)
				use = 1024 - position;

			applyFilter(filter, src, offset, use, buffer, position);

			byteLength -= use;
			offset += use;
			position += use;
			if (1024 === position) {
				this.#epd.spi.write(buffer);
				position = 0;
			}
		}
		buffer.position = position;
	}
	end() {
		const epd = this.#epd;

		if (this.#buffer.position)
			epd.spi.write(new Uint8Array(this.#buffer, 0, this.#buffer.position));

		epd.select.write(1);
		epd.writeCommand(IT8951_TCON_LD_IMG_END);

		const area = this.#area;
		epd.updateArea(area.x, area.y, area.width, area.height, this.#updateMode);

		delete area.continue;
	}
	continue() {
		const epd = this.#epd;

		epd.select.write(1);
		epd.writeCommand(IT8951_TCON_LD_IMG_END);

		this.#area.continue = true;
	}
	adaptInvalid(area) {
		if (area.x & 3) {
			area.w += area.x & 3;
			area.x &= ~3;
		}
		area.w = (area.w + 3) & ~3;

		if (this.#epd._rotation & 1) {
			if (area.y & 3) {
				area.h += area.y & 3;
				area.y &= ~3;
			}
			area.h = (area.h + 3) & ~3;
		}
	}
	refresh() {
		this.#epd.updateFull(UpdateMode.INIT);
	}
	get width() {
		return (this.#epd._rotation & 1) ? M5EPD_PANEL_H : M5EPD_PANEL_W;
	}
	get height() {
		return (this.#epd._rotation & 1) ? M5EPD_PANEL_W : M5EPD_PANEL_H;
	}
	set rotation(value) {
		const rotation = Math.idiv(value, 90);
		if (rotation & ~3)
			throw new Error;
		this.#epd._rotation = (rotation & 1) ? ((~rotation & 2) | 1) : rotation;
	}
	get rotation() {
		const rotation = this.#epd._rotation; 
		return 90 * ((rotation & 1) ? ((~rotation & 2) | 1) : rotation);
	}
	get pixelFormat() {
		return Bitmap.Gray16;
	}
	pixelsToBytes(pixels) {
		return (pixels + 1) >> 1;
	}
}

function applyFilter(filter, src, offset, byteLength, dst, position) @ "xs_applyFilter"; 

export default Display 
