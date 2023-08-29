/*
 * Copyright (c) 2023 Moddable Tech, Inc.
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
	BM8563 -I2C Real-Time-Clock
	https://datasheet.lcsc.com/lcsc/2207281130_GATEMODE-BM8563ESA_C269877.pdf
*/

const Register = Object.freeze({
  CONTROL_STATUS1: 0x00,
  CONTROL_STATUS2: 0x01,
  SECONDS: 0x02,
  CENTURY_BIT: 0b10000000,
});

class BM8563 {
  #io;
  #blockBuffer = new Uint8Array(7);

  constructor(options) {
    const { clock } = options;
    const io = (this.#io = new clock.io({
      hz: 400_000,
      address: 0x51,
      ...clock,
    }));

    try {
      // Status reset (twice)
      io.writeUint8(Register.CONTROL_STATUS1, 0x00);
      io.writeUint8(Register.CONTROL_STATUS1, 0x00);
      // Status2 reset
      io.writeUint8(Register.CONTROL_STATUS2, 0x00);
    } catch (e) {
      io.close();
      throw e;
    }
  }
  close() {
    this.#io.close();
    this.#io = undefined;
  }
  configure(options) {}
  get configuration() {
    return {};
  }
  get time() {
    const io = this.#io;
    const reg = this.#blockBuffer;
    io.readBuffer(Register.SECONDS, reg);
    const century = reg[5] & Register.CENTURY_BIT ? 100 : 0;
    // yr, mo, day, hr, min, sec
    return Date.UTC(
      bcdToDec(reg[6]) + century + 1900,
      bcdToDec(reg[5] & 0b00011111),
      bcdToDec(reg[3]),
      bcdToDec(reg[2] & 0x3f),
      bcdToDec(reg[1]),
      bcdToDec(reg[0] & 0x7f)
    );
  }
  set time(v) {
    let b = this.#blockBuffer;
    let now = new Date(v);
    let year = now.getUTCFullYear();
    b[0] = decToBcd(now.getUTCSeconds());
    b[1] = decToBcd(now.getUTCMinutes());
    b[2] = decToBcd(now.getUTCHours());
    b[3] = decToBcd(now.getUTCDate());
    b[4] = decToBcd(now.getUTCDay());
    b[5] = decToBcd(now.getUTCMonth());
    if (year >= 100) {
      b[5] |= Register.CENTURY_BIT;
    }
    b[6] = decToBcd(year % 100);

    this.#io.writeBuffer(Register.SECONDS, b);
  }
}

function decToBcd(d) {
  let val = d | 0;
  let v = (val / 10) | 0;
  v *= 16;
  v += val % 10;
  return v;
}
function bcdToDec(b) {
  let v = (b / 16) | 0;
  v *= 10;
  v += b % 16;
  return v;
}

export default BM8563;
