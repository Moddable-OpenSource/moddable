/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 * Copyright (c) 2019  Wilberforce
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
	HD44780 - LCD i2c module using PCF8574P i2c expander  in 4 Bit mode
*/

import I2C from "pins/i2c";
import Timer from "timer";

const displayPorts = {
	RS: 0x01,
	E: 0x04,
	D4: 0x10,
	D5: 0x20,
	D6: 0x40,
	D7: 0x80,
	backlight: 0x08,
	//RW: 0x20 not used
};

Object.freeze(displayPorts);

const LCD = {

	CHR: 1,
	CMD: 0,
	
	CLEARDISPLAY: 0x01,
	RETURNHOME: 0x02,
	ENTRYMODESET: 0x04,
	DISPLAYCONTROL: 0x08,
	CURSORSHIFT: 0x10,
	FUNCTIONSET: 0x20,
	SETCGRAMADDR: 0x40,
	SETDDRAMADDR: 0x80,

	//# flags for display entry mode
	ENTRYRIGHT: 0x00,
	ENTRYLEFT: 0x02,
	ENTRYSHIFTINCREMENT: 0x01,
	ENTRYSHIFTDECREMENT: 0x00,

	//# flags for display on/off control
	DISPLAYON: 0x04,
	DISPLAYOFF: 0x00,
	CURSORON: 0x02,
	CURSOROFF: 0x00,
	BLINKON: 0x01,
	BLINKOFF: 0x00,
	//BLOCKON: 0x80
	//BLOCKOFF: 0x00
	
	

	//# flags for display/cursor shift
	DISPLAYMOVE: 0x08,
	CURSORMOVE: 0x00,
	MOVERIGHT: 0x04,
	MOVELEFT: 0x00,

	//# flags for function set
	_8BITMODE: 0x10,
	_4BITMODE: 0x00,
	_2LINE: 0x08,
	_1LINE: 0x00,
	_5x10DOTS: 0x04,
	_5x8DOTS: 0x00,
};

Object.freeze(LCD);


class HD44780 extends I2C {
	constructor(dictionary) {

		super(Object.assign({
			address: 0x27
		}, dictionary));

		// Backlight state - default on
		this.backlight = displayPorts.backlight;

		this.write4(0x30, LCD.CMD); //initialization
		Timer.delay(200);
		this.write4(0x30, LCD.CMD); //initialization
		Timer.delay(100);
		this.write4(0x30, LCD.CMD); //initialization
		Timer.delay(100);
		this.write4(LCD.FUNCTIONSET | LCD._4BITMODE | LCD._2LINE | LCD._5x10DOTS, LCD.CMD); //4 bit - 2 line 5x7 matrix

		Timer.delay(10);
		this.write8(LCD.DISPLAYCONTROL | LCD.DISPLAYON, LCD.CMD); //turn cursor off 0x0E to enable cursor
		Timer.delay(10);
		this.write8(LCD.ENTRYMODESET | LCD.ENTRYLEFT, LCD.CMD); //shift cursor right
		Timer.delay(10);
		this.write8(LCD.CLEARDISPLAY, LCD.CMD); // LCD clear
		Timer.delay(10);
	}

	write4(nibble, cmd) {
		var msb = (nibble & 0xF0) | this.backlight; // Use upper 4 bit nibble
		this.write(msb | cmd);
		Timer.delay(1);
		this.write(msb | displayPorts.E | cmd);
		Timer.delay(1);
		this.write(msb | cmd);
		Timer.delay(1);
	}

	write8(byte, cmd) {
		this.write4(byte, cmd);
		this.write4(byte << 4, cmd);
		return this;
	}
	// print text
	print(str) {
		for (var i = 0; i < str.length; i++) {
			this.write8(str.charCodeAt(i), LCD.CHR);
		}
		return this;
	}
	// clear screen
	clear() {
		return this.write8(LCD.CLEARDISPLAY, LCD.CMD);
	}

	// flashing block for the current cursor, or underline
	cursor(block) {
		return this.write8(block ? 0x0F : 0x0E, LCD.CMD);
	}
	// set cursor pos, top left = 0,0
	setCursor(x, y) {
		var l = [0x00, 0x40, 0x14, 0x54];
		let bits = (l[y] + x);
		return this.write8(LCD.SETDDRAMADDR | bits, LCD.CMD);
	}
	// set special character 0..7, data is an array(8) of bytes, and then return to home addr
	createChar(ch, data) {
		this.write8(LCD.SETCGRAMADDR | ((ch & 7) << 3), LCD.CMD);
		for (var i = 0; i < 8; i++)
			this.write8(data[i], LCD.CHR);
		return this;
	}

	blink(state) {
		return this.write8(LCD.DISPLAYCONTROL | LCD.DISPLAYON | LCD.CURSORON | state ? LCD.BLINKON : LCD.BLINKOFF, LCD.CMD);
	}

	setBacklight(val) {
		if (val > 0) {
			this.backlight = displayPorts.backlight;
		} else {
			this.backlight = 0x00;
		}
		return this.write8(LCD.DISPLAYCONTROL, LCD.CMD);
	}
}
Object.freeze(HD44780.prototype);

export {
	HD44780 as
	default, HD44780, LCD
};