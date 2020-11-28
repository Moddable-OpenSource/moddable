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
import SMBus from "pins/smbus";

class DCDC {
  #parent;
  #register;
  constructor({ register, parent }) {
    this.#parent = parent;
    this.#register = register;
  }
  set voltage(v) {
    const vdata = v < 700 ? 0 : (v - 700) / 25;
    this.#parent.writeByte(
      this.#register,
      (this.#parent.readByte(this.#register) & 0x80) | (vdata & 0x7f)
    );
  }
  get voltage() {
    return (this.#parent.readByte(this.#register) & 0x7f) * 25 + 700;
  }
}

class LDO {
  #parent;
  #register;
  #offsetV;
  #offsetEn;
  constructor({ register, parent, offsetV, offsetEn }) {
    this.#parent = parent;
    this.#register = register;
    this.#offsetV = offsetV;
    this.#offsetEn = offsetEn;
  }

  set voltage(v) {
    const vdata = v > 3300 ? 15 : v / 100 - 18;
    const mask = ~(0xff << this.#offsetV);
    this.#parent.writeByte(
      this.#register,
      (this.#parent.readByte(this.#register) & mask) | (vdata << this.#offsetV)
    );
  }

  get voltage() {
    return (
      ((this.#parent.readByte(this.#register) >> this.#offsetV) + 18) * 100
    );
  }

  set enable(enable) {
    const mask = 0x01 << this.#offsetEn;
    if (enable) {
      this.#parent.writeByte(0x12, this.#parent.readByte(0x12) | mask);
    } else {
      this.#parent.writeByte(0x12, this.#parent.readByte(0x12) & ~mask);
    }
  }

  get enable() {
    return Boolean((this.#parent.readByte(0x12) >> this.#offsetEn) & 1);
  }
}

class GPIO {
	#register
	#parent
	#mask
	constructor({ register, parent, offset }) {
    this.#parent = parent;
		this.#register = register;
		this.#mask = 0x01 << offset;
	}

	get enable() {
		return Boolean(this.#parent.readByte(this.#register) & this.#mask);
	}

	set enable(enable) {
    let data = this.#parent.readByte(this.#register);
    if (enable) {
      data |= this.#mask;
    } else {
      data &= ~this.#mask;
    }
    this.#parent.writeByte(this.#register, data);
	}
}

export default class AXP192 extends SMBus {
  constructor(it) {
    super({ address: 0x34, ...it });
    this._dcdc1 = new DCDC({ register: 0x26, parent: this });
    this._dcdc2 = new DCDC({ register: 0x23, parent: this });
    this._dcdc3 = new DCDC({ register: 0x27, parent: this });
    this._ldo2 = new LDO({
      register: 0x34,
      parent: this,
      offsetV: 4,
      offsetEn: 2,
    });
    this._ldo3 = new LDO({
      register: 0x34,
      parent: this,
      offsetV: 0,
      offsetEn: 3,
		});
		this._gpio0 = new GPIO({ register: 0x94, parent: this, offset: 0 })
		this._gpio1 = new GPIO({ register: 0x94, parent: this, offset: 1 })
		this._gpio2 = new GPIO({ register: 0x94, parent: this, offset: 2 })
		this._gpio3 = new GPIO({ register: 0x96, parent: this, offset: 0 })
		this._gpio4 = new GPIO({ register: 0x96, parent: this, offset: 1 })
  }

  set chargeCurrent(state) {
    this.writeByte(0x33, (this.readByte(0x33) & 0xf0) | (state & 0x0f));
  }
  get batteryVoltage() {
    let data = this.readByte(0x78) << 4;
    data |= this.readByte(0x79);
    return data * 1.1 / 1000;
  }
  get batteryCurrent() {
    let currentIn = this.readByte(0x7a) << 5;
    currentIn |= this.readByte(0x7b);
    let currentOut = this.readByte(0x7c) << 5;
    currentOut |= this.readByte(0x7d);

    return (currentIn - currentOut) * 0.5;
  }
}

AXP192.CHARGE_CURRENT = {
  Ch_100mA: 0b0000,
  Ch_190mA: 0b0001,
  Ch_280mA: 0b0010,
  Ch_360mA: 0b0011,
  Ch_450mA: 0b0100,
  Ch_550mA: 0b0101,
  Ch_630mA: 0b0110,
  Ch_700mA: 0b0111,
  Ch_780mA: 0b1000,
  Ch_880mA: 0b1001,
  Ch_960mA: 0b1010,
  Ch_1000mA: 0b1011,
  Ch_1080mA: 0b1100,
  Ch_1160mA: 0b1101,
  Ch_1240mA: 0b1110,
  Ch_1320mA: 0b1111,
};
