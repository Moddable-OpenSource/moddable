/*
 *   Copyright (c) 2019 Shinya Ishikawa, 2024 RChikamura
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
    let vtemp; // 電圧>設定値の計算結果を代入する変数
    let mask = 0x80; // 電圧設定値をレジスタに書き込む際に使用。電圧設定のビットを0とし、それ以外を１とする。
    switch (
      this.#register // AXP2101にはDCDC1~5までがあるらしい(少なくとも4までは回路図にある)が、全部設定が違うので分岐
    ) {
      case 0x82: // DCDC1
        vtemp = v < 1500 ? 0 : v < 3400 ? (v - 1500) / 100 : 19;
        mask = 0xe0; // 下位5bitが電圧設定
        break;
      case 0x83: // DCDC2
        vtemp =
          v < 500
            ? 0
            : v <= 1200
            ? (v - 500) / 10
            : v <= 1540
            ? (v - 1200) / 20 + 70
            : 87;
        break;
      case 0x84: // DCDC3
        vtemp =
          v < 500
            ? 0
            : v <= 1200
            ? (v - 500) / 10
            : v <= 1540
            ? (v - 1200) / 20 + 70
            : v < 1600
            ? 87
            : v <= 3400
            ? (v - 1600) / 20 + 88
            : 107;
        break;
      case 0x85: // DCDC4
        vtemp =
          v < 500
            ? 0
            : v <= 1200
            ? (v - 500) / 10
            : v <= 1840
            ? (v - 1200) / 20 + 70
            : 102;
        break;
      case 0x86: // DCDC5
        vtemp = v < 1400 ? 0 : v < 3700 ? (v - 1400) / 100 : 23;
        mask = 0xe0; // 下位5bitが電圧設定
        break;
      default: // どれでもなければとりあえず0, maskもとりあえず標準(下位7bit)
        vtemp = 0;
        break;
    }
    const vdata = vtemp;
    this.#parent.writeByte(
      this.#register,
      (this.#parent.readByte(this.#register) & mask) | (vdata & ~mask)
    );
  }
  get voltage() {
    let v = 0;
    let vdata = this.#parent.readByte(this.#register);

    switch (
      this.#register // AXP2101にはDCDC1~5までがあるらしい(少なくとも4までは回路図にある)が、全部設定が違うので分岐
    ) {
      case 0x82: // DCDC1
        vdata &= 0x1f;
        v = vdata * 100 + 1500;
        break;
      case 0x83: // DCDC2
      case 0x85: // DCDC4 DCDC2とは上限値違いなだけのため、共通化
        vdata &= 0x7f;
        v = vdata <= 70 ? vdata * 10 + 500 : (vdata - 70) * 20 + 1200;
        break;
      case 0x84: // DCDC3
        vdata &= 0x7f;
        v =
          vdata <= 70
            ? vdata * 10 + 500
            : vdata <= 87
            ? (vdata - 70) * 20 + 1200
            : (vdata - 88) * 100 + 1600;
        break;
      case 0x86: // DCDC5
        vdata &= 0x1f;
        v = vdata * 100 + 1400;
        break;
      default: // どれでもなければ何もしない(値は0になる)
        break;
    }

    return v;
  }
}

class LDO {
  #parent;
  #register;
  #registerEn; // 追加 ただし、dldo2だけ0x91, それ以外は0x90なので、registerの値から条件演算子で選択する方式にする
  #offsetEn;
  // offsetVを削除し、#registerEnの自動判別コードを追加
  constructor({ register, parent, offsetEn }) {
    this.#parent = parent;
    this.#register = register;
    this.#registerEn = register == 0x9a ? 0x91 : 0x90;
    this.#offsetEn = offsetEn;
  }
  set voltage(v) {

    const vdata =
      this.#register == 0x98 || this.#register == 0x9a
        ? v < 500
          ? 0
          : v > 1400
            ? 19
            : (v - 500) / 50
        : v < 500
          ? 0
          : v > 3500
            ? 30
            : (v - 500) / 100;
    this.#parent.writeByte(
      this.#register,
      (this.#parent.readByte(this.#register) & 0x1f) | vdata
    ); //下位5bitに電圧設定を書き込み
  }

  get voltage() {
    let vdata = this.#parent.readByte(this.#register) & 0x1f;
    let v =
      this.#register == 0x98 || this.#register == 0x9a
        ? vdata * 50 + 500
        : vdata * 100 + 500;

    return v;
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

    //データシートとaxp192.jsのコードを参考に追加
    this._dcdc1 = new DCDC({ register: 0x82, parent: this });
    this._dcdc2 = new DCDC({ register: 0x83, parent: this });
    this._dcdc3 = new DCDC({ register: 0x84, parent: this });
    this._aldo1 = new LDO({ register: 0x92, parent: this, offsetEn: 0 });
    this._aldo2 = new LDO({ register: 0x93, parent: this, offsetEn: 1 });
    this._aldo3 = new LDO({ register: 0x94, parent: this, offsetEn: 2 });
    this._aldo4 = new LDO({ register: 0x95, parent: this, offsetEn: 3 });
    this._bldo1 = new LDO({ register: 0x96, parent: this, offsetEn: 4 });
    this._bldo2 = new LDO({ register: 0x97, parent: this, offsetEn: 5 });
    this._dldo1 = new LDO({ register: 0x99, parent: this, offsetEn: 7 });
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
    this.writeByte(0x10, 0b00110001);  // power off
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
    return ADC_LSB * this._read12Bit(0x5c);
  }

  getAXP173Temperature() {
    const ADC_LSB = 0.1;
    const OFFSET_DEG_C = -144.7;
    return ADC_LSB * this._read12Bit(0x5e) + OFFSET_DEG_C;
  }

  getTSTemperature() {
    const ADC_LSB = 0.1;
    const OFFSET_DEG_C = -144.7;
    return ADC_LSB * this._read12Bit(0x62) + OFFSET_DEG_C;
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
