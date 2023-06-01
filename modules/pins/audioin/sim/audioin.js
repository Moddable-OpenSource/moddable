/*
 * Copyright (c) 2023  Moddable Tech, Inc.
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

import Resource from "Resource";
import config from "mc/config";
import WavReader from "data/wavreader";

class AudioIn  {
	#wav;
	#reader;

	constructor() {
		this.#wav = new Resource("sim.wav");		
		this.#reader = new WavReader(this.#wav);
	}
	close() {
		this.#wav = this.#reader = undefined;
	}
	read(count, buffer = new ArrayBuffer(count * 2), offset = 0) {
		const output = new Int16Array(buffer, offset, count);
		let position = 0;
		while (count) {
			const use = Math.min(count, this.#reader.samples);
			this.#reader.getSamples(output.subarray(position, position + use), use);
			count -= use;
			position += use;

			if (0 === this.#reader.samples)
				this.#reader = new WavReader(this.#wav);		// loop
		}

		return buffer;
	}

	get sampleRate() {
		return this.#reader.sampleRate;
	}
	get bitsPerSample() {
		return 16;		// WavReader always outputs 16 bit samples
	}
	get numChannels() {
		return this.#reader.numChannels;
	}
}

export default AudioIn;
