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

export default class AXP2101 extends SMBus {
  constructor(it) {
    super({ address: 0x34, ...it });
    this._dledo1 = new LDO({
      register: 0x34,
      parent: this,
      offsetV: 5,
      offsetEn: 7,
    });
  }

  isACINExist() {
    return Boolean(this.readByte(0x00) & 0b10000000);
  }

  isACINAvailable() {
    return Boolean(this.readByte(0x00) & 0b01000000);
  }

  isVBUSExist() {
    return Boolean(this.readByte(0x00) & 0b00100000);
  }

  isVBUSAvailable() {
    return Boolean(this.readByte(0x00) & 0b00010000);
  }

  /**
   * @brief Get battery current direction
   * @returns true: battery charging, false: battery discharging
   */
  getBatteryCurrentDirection() {
    return Boolean(this.readByte(0x00) & 0b00000100);
  }

  isAXP173OverTemperature() {
    return Boolean(this.readByte(0x01) & 0b10000000);
  }

  isCharging() {
    return Boolean(this.readByte(0x01) & 0b01000000);
  }

  isBatteryExist() {
    return Boolean(this.readByte(0x01) & 0b00100000);
  }

  isChargeCSmaller() {
    return Boolean(this.readByte(0x01) & 0b00000100);
  }

  powerOff() {
    this.writeByte(0x10, this.readByte(0x10) | 0b00000001);
  }

  setChargeEnable(enable) {
    if (enable) {
      this.writeByte(0x33, this.readByte(0x33) | 0b10000000);
    } else {
      this.writeByte(0x33, this.readByte(0x33) | 0b10000000);
    }
  }

  setChargeCurrent(state) {
    this.writeByte(0x33, (this.readByte(0x33) & 0xf0) | (state & 0x0f));
  }

  setADCEnable(channel, enable) {
    const mask = 0x01 << channel;
    if (enable) {
      this.writeByte(0x82, this.readByte(0x82) | mask);
    } else {
      this.writeByte(0x82, this.readByte(0x82) & ~mask);
    }
  }

  setCoulometerEnable(channel, enable) {
    const mask = 0x01 << channel;
    if (enable) {
      this.writeByte(0x12, this.readByte(0x12) | mask);
    } else {
      this.writeByte(0x12, this.readByte(0x12) & ~mask);
    }
  }

  _getCoulometerCharge() {
    return this.readBlock(0xb0, 4);
  }

  _getCoulometerDischarge() {
    return this.readBlock(0xb4, 4);
  }

  getCoulometerCurrent() {
    const coin = this._getCoulometerCharge();
    const coout = this._getCoulometerDischarge();
    // data = 65536 * current_LSB * (coin - coout) / 3600 / ADC rate
    return (65536 * 0.5 * (coin - coout)) / 3600 / 25;
  }

  getBatteryVoltage() {
    const ADCLSB = 1.1 / 1000.0;
  }

  getBatteryLevel() {
    const voltage = this.getBatteryVoltage();
    const percentage = voltage < 3.248088 ? 0 : (voltage - 3.120712) * 100;
    return percentage <= 100 ? percentage : 100;
  }

  getBatteryPower() {
    const VOLTAGE_LSB = 1.1;
    const CURRENT_LCS = 0.5;
    return (VOLTAGE_LSB * CURRENT_LCS * this._read24Bit(0x70)) / 1000.0;
  }

  getVBUSVoltage() {
    const ADC_LSB = 1.7 / 1000.0;
    return ADC_LSB * this._read12Bit(0x5a);
  }

  getVBUSCurrent() {
    const ADC_LSB = 0.375;
    return ADC_LSB * this._read12Bit(0x5c)
  }

  getAXP173Temperature() {
    const ADC_LSB = 0.1;
    const OFFSET_DEG_C = -144.7;
    return ADC_LSB * this._read12Bit(0x5e) + OFFSET_DEG_C
  }

  getTSTemperature() {
    const ADC_LSB = 0.1;
    const OFFSET_DEG_C = -144.7;
    return ADC_LSB * this._read12Bit(0x62) + OFFSET_DEG_C
  }

  _read24Bit(address) {
    const buff = this.readBlock(address, 3);
    return (buff[0] << 16) + (buff[1] << 8) + buff[2];
  }

  _read12Bit(address) {
    const buff = this.readBlock(address, 2);
    return (buff[0] << 4) + buff[1];
  }
}

AXP2101.COULOMETER_CTRL = {
  COULOMETER_RESET: 0x05,
  COULOMETER_PAUSE: 0x06,
  COULOMETER_ENABLE: 0x07,
};
AXP2101.CHARGE_CURRENT = {
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
