/*
 *   Copyright (c) 2019-2024 Shinya Ishikawa, 2024 RChikamura
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

function vToByte(v, steps, points) {
  if (v < points[0]) {
    return 0;
  }
  let base = 0;
  for (let i = 1; i < points.length; i++) {
    const step = steps[i - 1];
    if (v < points[i]) {
      return base + (v - points[i - 1]) * step;
    }
    base += (points[i] - points[i - 1]) * step;
  }
  return base;
}

function byteToV(byte, points, steps) {
  if (byte <= 0) {
    return points[0]
  }
  let base = 0;
  for (let i = 1; i < points.length; i++) {
    const step = steps[i - 1];
    const delta = (points[i] - points[i - 1]) * step;
    if (byte <= base + delta) {
      return points[i - 1] + (byte - base) / step;
    }
    base += delta;
  }
  // should not be here since the byte value is over the maximum
  return points[points.length - 1] + (byte - base) / steps[steps.length - 1];

}
class DCDC {
  #parent;
  #registerV;
  #mask;
  #points;
  #steps;

  constructor({ registerV, parent, mask, points, steps }) {
    this.#parent = parent;
    this.#registerV = registerV;
    this.#mask = mask;
    this.#points = points;
    this.#steps = steps;
  }

  set voltage(v) {
    const byte = vToByte(v, this.#points, this.#steps);
    this.#parent.writeByte(
      this.#registerV,
      (this.#parent.readByte(this.#registerV) & this.#mask) |
      (byte & ~this.#mask)
    );
  }
  get voltage() {
    let byte = this.#parent.readByte(this.#registerV);
    return byteToV(byte & this.#mask, this.#points, this.#steps)
  }
}

class LDO {
  #parent;
  #registerV;
  #registerEn;
  #offsetEn;
  #points;
  #steps;
  constructor({ registerV, registerEn, parent, offsetEn, points, steps }) {
    this.#parent = parent;
    this.#registerV = registerV;
    this.#registerEn = registerEn;
    this.#offsetEn = offsetEn;
    this.#points = points;
    this.#steps = steps;
  }

  set voltage(v) {
    const byte = vToByte(v, this.#points, this.#steps)
    this.#parent.writeByte(
      this.#registerV,
      (this.#parent.readByte(this.#registerV) & 0x1f) | byte
    );
  }

  get voltage() {
    const byte = this.#parent.readByte(this.#registerV) & 0x1f;
    return byteToV(byte);
  }

  set enable(enable) {
    const mask = 0x01 << this.#offsetEn;
    if (enable) {
      this.#parent.writeByte(
        this.#registerEn,
        this.#parent.readByte(this.#registerEn) | mask
      );
    } else {
      this.#parent.writeByte(
        this.#registerEn,
        this.#parent.readByte(this.#registerEn) & ~mask
      );
    }
  }

  get enable() {
    return Boolean(
      (this.#parent.readByte(this.#registerEn) >> this.#offsetEn) & 1
    );
  }
}

export default class AXP2101 extends SMBus {
  constructor(it) {
    super({ address: 0x34, ...it });

    this._dcdc1 = new DCDC({
      registerV: 0x82,
      parent: this,
      points: [1500, 3400],
      steps: [1 / 100],
      mask: 0xe0,
    });
    this._dcdc2 = new DCDC({
      registerV: 0x83,
      parent: this,
      points: [500, 1200, 1600],
      steps: [1 / 10, 1 / 20],
      mask: 0x80,
    });
    this._dcdc3 = new DCDC({
      registerV: 0x84,
      parent: this,
      points: [500, 1200, 1540, 1600, 3400],
      steps: [1 / 10, 1 / 20, 0, 1 / 20],
      mask: 0x80,
    });
    this._dcdc4 = new DCDC({
      registerV: 0x85,
      parent: this,
      points: [500, 1200, 1840],
      steps: [1 / 10, 1 / 20],
      mask: 0x80,
    });
    this._dcdc5 = new DCDC({
      registerV: 0x86,
      parent: this,
      points: [1400, 3700],
      steps: [1 / 100],
      mask: 0xe0,
    });

    const PARAM_A = {
      parent: this,
      points: [500, 1400],
      steps: [1 / 50],
    };
    const PARAM_B = {
      parent: this,
      points: [500, 3500],
      steps: [1 / 100],
    };
    this._aldo1 = new LDO({
      ...PARAM_B,
      registerV: 0x92,
      registerEn: 0x90,
      offsetEn: 0,
    });
    this._aldo2 = new LDO({
      ...PARAM_B,
      registerV: 0x93,
      registerEn: 0x90,
      offsetEn: 1,
    });
    this._aldo3 = new LDO({
      ...PARAM_B,
      registerV: 0x94,
      registerEn: 0x90,
      offsetEn: 2,
    });
    this._aldo4 = new LDO({
      ...PARAM_B,
      registerV: 0x95,
      registerEn: 0x90,
      offsetEn: 3,
    });
    this._bldo1 = new LDO({
      ...PARAM_B,
      registerV: 0x96,
      registerEn: 0x90,
      offsetEn: 4,
    });
    this._bldo2 = new LDO({
      ...PARAM_B,
      registerV: 0x97,
      registerEn: 0x90,
      offsetEn: 5,
    });
    this._dldo1 = new LDO({
      ...PARAM_B,
      registerV: 0x99,
      registerEn: 0x90,
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
    this.writeByte(0x10, this.readByte(0x10) | 0b00000010); // POWERON Negative Edge IRQ(ponne_irq_en) enable
    this.writeByte(0x25, 0b00011011); // sleep and wait for wakeup
    Timer.delay(100);
    this.writeByte(0x10, 0b00110001); // power off
  }

  setChargeEnable(_enable) {
    throw new Error("not implemented");
  }

  setChargeCurrent(_state) {
    throw new Error("not implemented");
  }

  setADCEnable(_channel, _enable) {
    throw new Error("not implemented");
  }

  setCoulometerEnable(channel, enable) {
    throw new Error("not implemented");
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
    // TODO: implement
  }

  getBatteryLevel() {
    const voltage = this.getBatteryVoltage();
    const percentage = voltage < 3.248088 ? 0 : (voltage - 3.120712) * 100;
    return percentage <= 100 ? percentage : 100;
  }

  getBatteryPower() {
    const VOLTAGE_LSB = 1.1;
    const CURRENT_LCS = 0.5;
    return (VOLTAGE_LSB * CURRENT_LCS * this.#read24Bit(0x70)) / 1000.0;
  }

  getVBUSVoltage() {
    const ADC_LSB = 1.7 / 1000.0;
    return ADC_LSB * this.#read12Bit(0x5a);
  }

  getVBUSCurrent() {
    const ADC_LSB = 0.375;
    return ADC_LSB * this.#read12Bit(0x5c);
  }

  getAXP173Temperature() {
    const ADC_LSB = 0.1;
    const OFFSET_DEG_C = -144.7;
    return ADC_LSB * this.#read12Bit(0x5e) + OFFSET_DEG_C;
  }

  getTSTemperature() {
    const ADC_LSB = 0.1;
    const OFFSET_DEG_C = -144.7;
    return ADC_LSB * this.#read12Bit(0x62) + OFFSET_DEG_C;
  }

  #read24Bit(address) {
    const buff = this.readBlock(address, 3);
    return (buff[0] << 16) + (buff[1] << 8) + buff[2];
  }

  #read12Bit(address) {
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
