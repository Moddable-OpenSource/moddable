/*
 *   Copyright (c) 2019 Shinya Ishikawa
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
import SMBus from 'pins/smbus';
import Timer from 'timer';

const kCHG_100mA = 0

export default class AXP192 extends SMBus {
	constructor(it) {
		if (it.address == null) {
			it.address = 0x34
		}
		super(it);
		this.initialize();
	}

	//set led state(GPIO high active,set 1 to enable amplifier)
	setSpeakerEnable(state) {
		const register = 0x94;
		const gpioBit = 0x04;
		let data = this.readByte(register);
		if (state) {
			data |= gpioBit;
		}
		else {
			data &= ~gpioBit;
		}
		this.writeByte(register, data);
	}

	setVoltage(v) {
		if (v >= 3000 && v <= 3400) {
			this.setDCVoltage(0, v)
		}
	}

	setLcdVoltage(v) {
		if (v >= 2500 && v <= 3300) {
			this.setDCVoltage(2, v)
		}
	}

	setLdoVoltage(ch, v) {
		const vdata = (v > 3300) ? 15 : (v / 100) - 18;
		switch (ch) {
			case 2:
				this.writeByte(0x34, (this.readByte(0x28) & 0x0f) | vdata << 4)
				break
			case 3:
				this.writeByte(0x34, (this.readByte(0x28) & 0xf0) | vdata)
				break
		}

	}

	setDCVoltage(ch, v) {
		if (ch > 2) {
			return
		}
		const vdata = (v < 700) ? 0 : (v - 700) / 25
		let register
		switch (ch) {
			case 0:
				register = 0x26
				break
			case 1:
				register = 0x25
				break
			case 2:
				register = 0x27
				break
		}
		this.writeByte(register, (this.readByte(register) & 0x80) | (vdata & 0x7f))
	}

	setLdoEnable(n, enable) {
		let mask = 0x01
		if (n < 2 || n > 3) {
			return
		}
		mask <<= n
		if (enable) {
			this.writeByte(0x12, this.readByte(0x12) | mask)
		} else {
			this.writeByte(0x12, this.readByte(0x12) & ~mask)
		}
	}

	setLcdReset(enable) {
		const register = 0x96
		const gpioBit = 0x02
		let data = this.readByte(register)
		if (enable) {
			data |= gpioBit
		} else {
			data &= ~gpioBit
		}
		this.writeByte(register, data)
	}

	setBusPowerMode(mode) {
		if (mode == 0) {
			this.writeByte(0x91, (this.readByte(0x91) & 0x0f) | 0xf0)
			this.writeByte(0x90, (this.readByte(0x90) & 0xf8) | 0x02) //set GPIO0 to LDO OUTPUT , pullup N_VBUSEN to disable supply from BUS_5V
			this.writeByte(0x12, (this.readByte(0x12) | 0x40)) //set EXTEN to enable 5v boost
		} else {
			this.writeByte(0x12, (this.readByte(0x12) & 0xbf)) //set EXTEN to disable 5v boost
			this.writeByte(0x90, (this.readByte(0x90) & 0xf8) | 0x01) //set GPIO0 to float , using enternal pulldown resistor to enable supply from BUS_5VS
		}
	}

	setChargeCurrent(state) {
		this.writeByte(0x33, (this.readByte(0x33) & 0xf0) | (state & 0x0f))
	}

	/**
	 * initialize axp192
	 */
	initialize() {
		this.writeByte(0x30, (this.readByte(0x30) & 0x04) | 0x02) //AXP192 30H
		this.writeByte(0x92, this.readByte(0x92) & 0xf8) //AXP192 GPIO1:OD OUTPUT

		this.writeByte(0x93, this.readByte(0x93) & 0xf8) //AXP192 GPIO2:OD OUTPUT
		this.writeByte(0x35, (this.readByte(0x35) & 0x1c) | 0xa3)//AXP192 RTC CHG
		this.setVoltage(3350) // Voltage 3.35V
		this.setLcdVoltage(2800) // LCD backlight voltage 2.80V
		this.setLdoVoltage(2, 3300) //Periph power voltage preset (LCD_logic, SD card)
		this.setLdoVoltage(3, 2000) //Vibrator power voltage preset
		this.setLdoEnable(2, true)
		this.setChargeCurrent(kCHG_100mA)

		//AXP192 GPIO4
		this.writeByte(0x95, (this.readByte(0x95) & 0x72) | 0x84)
		this.writeByte(0x36, 0x4c)
		this.writeByte(0x82, 0xff)

		this.setLcdReset(0);
		Timer.delay(20);
		this.setLcdReset(1);
		Timer.delay(20);

		this.setBusPowerMode(0); //  bus power mode_output
		Timer.delay(200);
	}

	/**
	 * sets the screen brightness
	 * @param {*} brightness brightness between 7-15
	 */
	setBrightness(brightness) {
		const b = (brightness & 0x0f) << 4;
		this.writeByte(0x28, b);
	}
}