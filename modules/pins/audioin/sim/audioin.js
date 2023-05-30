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

import {File} from "file";
import config from "mc/config";
import WavReader from "data/wavreader";

class AudioIn  {
	#wav;
	#reader;

	constructor() {
		if (!config.audioin.sim)
			throw new Error("audo input simulator file not configured");
		const file = new File(config.audioin.sim);
		this.#wav = file.read(ArrayBuffer, file.length);
		file.close();
		
		this.#reader = new WavReader(this.#wav);
	}
	close() {
		this.#wav = this.#reader = undefined;
	}
	read(count, buffer = new ArrayBuffer(count * 2), offset = 0) {
		const output = new Int16Array(buffer, offset, count);
		let position = 0;
		while (count) {
			const remain = (this.#reader.waveSize - this.#reader.position) >> 1
			const use = Math.min(count, remain);
			this.#reader.getSamples(output.subarray(position, position + use), use);
			count -= use;
			position += use;

			if (this.#reader.waveSize === this.#reader.position)
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
