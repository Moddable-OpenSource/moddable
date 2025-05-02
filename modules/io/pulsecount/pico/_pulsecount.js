/*
 * Copyright (c) 2025 Moddable Tech, Inc.
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

class PulseCount {
	#signal;
	#control;
	#value = 0;
	#prevNext = 0;
	#store = 0;
	#isValid = [ 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 ];
	#onReadable;

	constructor(options) {
		const Digital = device.io.Digital;		// should be passed in to allow expanders

		this.#signal = new Digital({
			pin: options.signal,
			mode: Digital.InputPullUp,
			edge: Digital.Rising | Digital.Falling,
			onReadable: () => this.#trigger()
		});
		this.#control = new Digital({
			pin: options.control,
			mode: Digital.InputPullUp,
			edge: Digital.Rising | Digital.Falling,
			onReadable: () => this.#trigger()
		});
		this.#onReadable = options.onReadable;
	}
	#trigger() {
		const val = this.#sample();
		if (val) {
			this.#value += val;
			this.#onReadable?.();
		}
	}
	#sample() {
		this.#prevNext <<= 2;
		this.#prevNext |= (this.#signal.read() ? 0x02 : 0);
		this.#prevNext |= (this.#control.read() ? 0x01 : 0);
		this.#prevNext &= 0x0F;

		if (this.#isValid[this.#prevNext]) {
			this.#store <<= 4;
			this.#store |= this.#prevNext;
			if ((this.#store & 0xFF) == 0x2B)
				return -1;
			if ((this.#store & 0xFF) == 0x17)
				return 1;
		}
		return 0;
	}
	close() {
		this.#signal?.close();
		this.#control?.close();
	}
	read() { return this.#value; }
	write(count) { this.#value = count; };

    get format() {
        return "number";
    }

    set format(value) {
        if ("number" !== value)
            throw new RangeError;
    }

};

export default PulseCount;
