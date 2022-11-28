/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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
  PAJ7620U2 Gesture recognition sensor
  https://datasheetspdf.com/pdf-file/1309990/PixArt/PAJ7620U2/1
*/

import Timer from "timer";

const REGISTERS = Object.freeze({
  BANK_SEL: 0xef,
  // Bank 0 Register
  PART_ID_LOW: 0x00,
  GES_RESULT_0: 0x43,
  GES_RESULT_1: 0x44,
  // Bank 1 Register
  LENS_ORIENTATION: 0x04,
  OPERATION_ENABLE: 0x72,
});

const ENTRY_TIME = 0;
const EXIT_TIME = 200;

const GESTURE_FLAG = Object.freeze({
  UP: 0x01,
  DOWN: 0x02,
  LEFT: 0x04,
  RIGHT: 0x08,
  FORWARD: 0x10,
  BACKWARD: 0x20,
  CLOCKWISE: 0x40,
  ANICLOCKWISE: 0x080,
  WAVE: 0x01,
});

const INIT_REGISTERS = Object.freeze([
  { addr: 0xef, val: 0x00 },
  { addr: 0x41, val: 0x00 },
  { addr: 0x42, val: 0x00 },
  { addr: 0x37, val: 0x07 },
  { addr: 0x38, val: 0x17 },
  { addr: 0x39, val: 0x06 },
  { addr: 0x42, val: 0x01 },
  { addr: 0x46, val: 0x2d },
  { addr: 0x47, val: 0x0f },
  { addr: 0x48, val: 0x3c },
  { addr: 0x49, val: 0x00 },
  { addr: 0x4a, val: 0x1e },
  { addr: 0x4c, val: 0x22 },
  { addr: 0x51, val: 0x10 },
  { addr: 0x5e, val: 0x10 },
  { addr: 0x60, val: 0x27 },
  { addr: 0x80, val: 0x42 },
  { addr: 0x81, val: 0x44 },
  { addr: 0x82, val: 0x04 },
  { addr: 0x8b, val: 0x01 },
  { addr: 0x90, val: 0x06 },
  { addr: 0x95, val: 0x0a },
  { addr: 0x96, val: 0x0c },
  { addr: 0x97, val: 0x05 },
  { addr: 0x9a, val: 0x14 },
  { addr: 0x9c, val: 0x3f },
  { addr: 0xa5, val: 0x19 },
  { addr: 0xcc, val: 0x19 },
  { addr: 0xcd, val: 0x0b },
  { addr: 0xce, val: 0x13 },
  { addr: 0xcf, val: 0x64 },
  { addr: 0xd0, val: 0x21 },
  { addr: 0xef, val: 0x01 },
  { addr: 0x02, val: 0x0f },
  { addr: 0x03, val: 0x10 },
  { addr: 0x04, val: 0x02 },
  { addr: 0x25, val: 0x01 },
  { addr: 0x27, val: 0x39 },
  { addr: 0x28, val: 0x7f },
  { addr: 0x29, val: 0x08 },
  { addr: 0x3e, val: 0xff },
  { addr: 0x5e, val: 0x3d },
  { addr: 0x65, val: 0x96 },
  { addr: 0x67, val: 0x97 },
  { addr: 0x69, val: 0xcd },
  { addr: 0x6a, val: 0x01 },
  { addr: 0x6d, val: 0x2c },
  { addr: 0x6e, val: 0x01 },
  { addr: 0x72, val: 0x01 },
  { addr: 0x73, val: 0x35 },
  { addr: 0x74, val: 0x00 },
  { addr: 0x77, val: 0x01 },
  { addr: 0xef, val: 0x00 },
  { addr: 0x41, val: 0xff },
  { addr: 0x42, val: 0x01 },
], true);

const GESTURE_MODE_REGISTERS = Object.freeze([
  { addr: 0xef, val: 0x00 },
  { addr: 0x41, val: 0x00 },
  { addr: 0x42, val: 0x00 },
  { addr: 0x48, val: 0x3c },
  { addr: 0x49, val: 0x00 },
  { addr: 0x51, val: 0x10 },
  { addr: 0x83, val: 0x20 },
  { addr: 0x9f, val: 0xf9 },
  { addr: 0xef, val: 0x01 },
  { addr: 0x01, val: 0x1e },
  { addr: 0x02, val: 0x0f },
  { addr: 0x03, val: 0x10 },
  { addr: 0x04, val: 0x02 },
  { addr: 0x41, val: 0x40 },
  { addr: 0x43, val: 0x30 },
  { addr: 0x65, val: 0x96 },
  { addr: 0x66, val: 0x00 },
  { addr: 0x67, val: 0x97 },
  { addr: 0x68, val: 0x01 },
  { addr: 0x69, val: 0xcd },
  { addr: 0x6a, val: 0x01 },
  { addr: 0x6b, val: 0xb0 },
  { addr: 0x6c, val: 0x04 },
  { addr: 0x6d, val: 0x2c },
  { addr: 0x6e, val: 0x01 },
  { addr: 0x74, val: 0x00 },
  { addr: 0xef, val: 0x00 },
  { addr: 0x41, val: 0xff },
  { addr: 0x42, val: 0x01 },
], true);

class PAJ7620U2 {
  #io;
  #onError;

  constructor(options) {
    const io = this.#io = new options.sensor.io({
      address: 0x73,
      hz: 400_000,
      ...options.sensor
    });

    this.#onError = options.onError;

    Timer.delay(100);

    this.#selectBank(0);

    const partid = this.#io.readUint16(REGISTERS.PART_ID_LOW);
    if (partid != 0x7620) throw new Error("ERR_IC_VERSION");

    this.#initializeDeviceSettings();
    this.#setGestureMode();

    this.#selectBank(0);

    this.configure({
      enabled: true,
      invert: false,
      flip: "none"
    });
  }

  configure(options) {
    const io = this.#io;

    this.#selectBank(1);

    if ("enabled" in options) {
      io.writeUint8(REGISTERS.OPERATION_ENABLE, Number(options.enabled));
    }

    if ("invert" in options) {
      delete io.invert;
      if (options.invert)
        io.invert = true;
    }

    let value = options.flip;
    if (value) {
      let data = io.readUint8(REGISTERS.LENS_ORIENTATION);

      if ("h" === value) {
        data ^= 1 << (io.invert ? 1 : 0);
      } else if ("v" === value) {
        data ^= 1 << (io.invert ? 0 : 1);
      } else if ("hv" === value) {
        data ^= 0b11;
      }
      io.writeUint8(REGISTERS.LENS_ORIENTATION, data);
    }
    this.#selectBank(0);
  }

  close() {
    this.#io?.close();
    this.#io = undefined;
  }

  #selectBank(bank) {
    this.#io.writeUint8(REGISTERS.BANK_SEL, bank);
  }

  #initializeDeviceSettings() {
    INIT_REGISTERS.forEach((reg) => {
      this.#io.writeUint8(reg.addr, reg.val);
    });
    this.#selectBank(0);
  }

  #setGestureMode() {
    GESTURE_MODE_REGISTERS.forEach((reg) => {
      this.#io.writeUint8(reg.addr, reg.val);
    });
    this.#selectBank(0);
  }

  sample() {
    return this.#getGesture();
  }

  /* GestureMode method */

  #forwardBackwardGestureCheck(gesture) {
    let result = gesture;

    Timer.delay(ENTRY_TIME);
    let res0 = this.#io.readUint8(REGISTERS.GES_RESULT_0);

    if (res0 == GESTURE_FLAG.FORWARD) {
      Timer.delay(EXIT_TIME);
      result = "Forward";
    } else if (res0 == GESTURE_FLAG.BACKWARD) {
      Timer.delay(EXIT_TIME);
      result = "Backward";
    }

    return result;
  }

  #getGesture() {
    const io = this.#io;
    let res1 = io.readUint8(REGISTERS.GES_RESULT_1);
    if (res1 == GESTURE_FLAG.WAVE) {
      return "Wave";
    }

    let res0 = io.readUint8(REGISTERS.GES_RESULT_0);
    if (!res0) {
      return undefined;
    }

    let result = undefined;
    switch (res0) {
      case GESTURE_FLAG.UP:
        result = this.#forwardBackwardGestureCheck(io.invert ? "Right" : "Up");
        break;
      case GESTURE_FLAG.DOWN:
        result = this.#forwardBackwardGestureCheck(io.invert ? "Left" : "Down");
        break;
      case GESTURE_FLAG.LEFT:
        result = this.#forwardBackwardGestureCheck(io.invert ? "Up" : "Left");
        break;
      case GESTURE_FLAG.RIGHT:
        result = this.#forwardBackwardGestureCheck(io.invert ? "Down" : "Right");
        break;
      case GESTURE_FLAG.FORWARD:
        Timer.delay(EXIT_TIME);
        result = "Forward";
        break;
      case GESTURE_FLAG.BACKWARD:
        Timer.delay(EXIT_TIME);
        result = "Backward";
        break;
      case GESTURE_FLAG.CLOCKWISE:
        result = "Clockwise";
        break;
      case GESTURE_FLAG.ANICLOCKWISE:
        result = "Anti-Clockwise";
        break;
      default:
        break;
    }
    return result;
  }
}

export default PAJ7620U2;
