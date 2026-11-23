/*
 * Copyright (c) 2024-2025 Moddable Tech, Inc.
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

import Timer from "timer";
import ES8311 from "embedded:peripheral/Audio/ES8311";
import AudioInput from "embedded:io/audio/in";
import AudioOutput from "embedded:io/audio/out";

let owner;
let expander;

export function setAudioExpander(value) {
	expander = value;
}

function audioClass(Base, output) {
	return class extends Base {
		#amplifier;
		#codec;
		#closed = false;
		#started = false;

		constructor(options = {}) {
			if ((options.sampleRate ?? 16000) !== 16000 ||
				(options.channels ?? 1) !== 1 || (options.bitsPerSample ?? 16) !== 16)
				throw new RangeError("16 kHz mono 16-bit audio required");

			// Input and output both drive the codec's I2S clock pins.
			if (owner)
				throw new Error("close the active M5StopWatch audio input or output first");

			super(options);
			owner = this;
			try {
				if (output) {
					const Digital = device.io.Digital;
					this.#amplifier = new Digital({pin: 14, mode: Digital.Output, initialValue: 0});
					this.#amplifier.write(0);
				}
				expander.write(3, 1);
				Timer.delay(10);
				this.#codec = new ES8311({...device.i2c.default, io: device.io.SMBus});
				this.#codec.configure({input: true, output: true});
			}
			catch (error) {
				try {
					this.close();
				}
				catch {
				}
				throw error;
			}
		}

		start() {
			if (this.#closed)
				throw new Error("audio closed");
			if (this.#started)
				return;

			try {
				super.start();
				this.#codec.start();
				if (output) {
					expander.write(10, 1);
					this.#amplifier.write(1);
					Timer.delay(10);
				}
				this.#started = true;
			}
			catch (error) {
				try {
					this.close();
				}
				catch {
				}
				throw error;
			}
		}

		stop() {
			if (this.#closed)
				throw new Error("audio closed");
			if (!this.#started)
				return;

			try {
				this.#mute();
				super.stop();
				this.#started = false;
			}
			catch (error) {
				try {
					this.close();
				}
				catch {
				}
				throw error;
			}
		}

		close() {
			if (this.#closed)
				return;
			this.#closed = true;

			try {
				try {
					this.#mute();
				}
				finally {
					this.#amplifier?.close();
				}
			}
			finally {
				try {
					super.close();
				}
				finally {
					try {
						this.#codec?.close();
					}
					finally {
						owner = undefined;
					}
				}
			}
		}

		#mute() {
			if (this.#amplifier) {
				try {
					this.#amplifier.write(0);
				}
				finally {
					expander.write(10, 0);
				}
			}
		}

		static {
			this.prototype[Symbol.dispose] = this.prototype.close;
		}
	};
}

const Input = audioClass(AudioInput, false);
const Output = audioClass(AudioOutput, true);
Output.Async = audioClass(AudioOutput.Async, true);

export default {Input, Output};
