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
import AudioOut from "pins/audioout";
import Resource from "Resource";
import config from "mc/config";
import Timer from "timer";

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

class M5CoreS3Button {		// M5StackCoreTouch calls write when button changes 
	#value = 0;
	read() {
		return this.#value;
	}
	write(value) {
		if (this.#value === value)
			return;
		this.#value = value;
		this.onChanged?.();
	}
}

export default function (done) {
	// buttons
	globalThis.button = {
		a: new M5CoreS3Button,
		b: new M5CoreS3Button,
		c: new M5CoreS3Button,
	};

  // power
  globalThis.power = new Power();

	// start-up sound
	if (config.startupSound) {
    const speaker = new AudioOut({streams: 1});
		speaker.callback = function () {
			this.stop();
			this.close();
			this.done();
		};
		speaker.done = done;
		done = undefined;

		speaker.enqueue(0, AudioOut.Samples, new Resource(config.startupSound));
		speaker.enqueue(0, AudioOut.Callback, 0);
		speaker.start();
	}

  done?.();
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

    this.resetLcd()
  }

  resetLcd() {
    this.expander.writeByteMask(0x03, 0, 0b11011111);
    Timer.delay(20);
    this.expander.writeByteMask(0x03, 0b00100000, 0xff);
  }
}

/**
 * AW9523 Expander IC
 */
class AW9523 extends SMBus {
  constructor(it) {
    super({ address: 0x58, ...it });
  }

  writeByteMask(address, data, mask) {
    const tmp = this.readByte(address);
    const newData = (tmp & mask) | data;
    trace(`tmp: ${tmp}, mask: ${mask}, writing: ${newData}\n`);
    this.writeByte(address, newData);
  }
}

function nop() {}
