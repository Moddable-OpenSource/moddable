/*
 * Copyright (c) 2020-2022  Moddable Tech, Inc.
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

import AXP2101 from "axp2101";
import SMBus from "pins/smbus";
import Timer from "timer";
import { getSampleRate } from "get-sample-rate";

const INTERNAL_I2C = Object.freeze({
  sda: 12,
  scl: 11,
});

const state = {
  handleRotation: nop,
};

globalThis.Host = {
  Backlight: class {
    constructor(brightness = 100) {
      this.write(brightness);
    }
    write(value) {
      if (undefined !== globalThis.power) globalThis.power.brightness = value;
    }
    close() {}
  },
};

export default function (done) {
  // power
  globalThis.power = new Power();
  globalThis.amp = new AW88298();
  done?.();
}

class AW88298 extends SMBus {
  constructor() {
    super({ address: 0x36, ...INTERNAL_I2C });
    const rate_tbl = [4, 5, 6, 8, 10, 11, 15, 20, 22, 44];
    let reg0x06_value = 0;
    /**
     * @note 11025Hz is not available for the slight gap between the clock of ESP32S3 and AW88298 PLL
     * @fixme should reset sampleRate if the different value specified in AudioOut#constructor
     */
    let sample_rate = getSampleRate();
    let rate = Math.round((sample_rate + 1102) / 2205);
    while (rate > rate_tbl[reg0x06_value] && ++reg0x06_value < rate_tbl.length);
    reg0x06_value |= 0x14c0; // I2SRXEN=1 CHSEL=01(left) I2SFS=11(32bits)
    this.writeWord(0x05, 0x0008, true); // RMSE=0 HAGCE=0 HDCCE=0 HMUTE=0
    this.writeWord(0x06, reg0x06_value, true);
    this.writeWord(0x61, 0x0673, true); // boost mode disabled
    this.writeWord(0x0c, 0x1064, true); // volume setting (full volume)
    this.writeWord(0x04, 0x4040, true); // I2SEN=1 AMPPD=0 PWDN=0
  }
}

class Power extends AXP2101 {
  constructor() {
    super(INTERNAL_I2C);
    this.writeByte(0x90, 0xbf);
    this.writeByte(0x92, 13);
    this.writeByte(0x93, 28);
    this.writeByte(0x94, 28);
    this.writeByte(0x95, 28);
    this.writeByte(0x27, 0);
    this.writeByte(0x69, 0x11);
    this.writeByte(0x10, 0x30);

    this.expander = new AW9523(INTERNAL_I2C);
    this.expander.writeByte(0x02, 0b00000111);
    this.expander.writeByte(0x03, 0b10000011);
    this.expander.writeByte(0x04, 0b00011000);
    this.expander.writeByte(0x05, 0b00001100);
    this.expander.writeByte(0x11, 0b00010000);
    this.expander.writeByte(0x12, 0b11111111);
    this.expander.writeByte(0x13, 0b11111111);

    // this.resetLcd()
    this.writeByte(0x03, 0b10000011);
    Timer.delay(20);
    this.writeByte(0x03, 0b10100011);
  }

  resetLcd() {
    this.expander.writeByteMask(0x03, 0, 0b11011111);
    Timer.delay(20);
    // FIXME: read byte is 0x00 despite of write op above. something wrong
    this.expander.writeByteMask(0x03, 0b00100000, 0xff);
  }
}

class AW9523 extends SMBus {
  constructor(it) {
    super({ address: 0x58, ...it });
  }

  writeByteMask(address, data, mask) {
    const tmp = this.readByte(address);
    const newData = (tmp & mask) | data;
    trace(`tmp: ${tmp}, mask: ${mask}, writing: ${newData}\n`);
    this.writeByte(newData);
  }
}

function nop() {}
