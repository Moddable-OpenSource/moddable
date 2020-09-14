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

export const kCHG_100mA = 0

export default class AXP192 extends SMBus {
	constructor(it) {
		if (it.address == null) {
			it.address = 0x34
		}
		super(it);
		if (it.onInit != null) {
			it.onInit.apply(this)
	}
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

	isLdoEnable(n) {
		if (n < 2 || n > 3) {
			return
		}
		return Boolean((this.readByte(0x12) >> n) & 1)
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
	 * @deprecated use onInit() instead
	 */
	initialize() {
		// NOTE: below is M5StickC specific flow left for backward compatibility
		this.write(0x10, 0xff); // OLED VPP Enable
		this.write(0x28, 0xff); // Enable LDO2&LDO3, LED&TFT 3.3V
		this.write(0x82, 0xff); // Enable all the ADCs
		this.write(0x33, 0xc0); // Enable Charging, 100mA, 4.2V End at 0.9
		this.write(0xB8, 0x80); // Enable Colume Counter
		this.write(0x12, 0x4d); // Enable DC-DC1, OLED VDD, 5B V EXT
		this.write(0x36, 0x5c); // PEK
		this.write(0x90, 0x02); // gpio0
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