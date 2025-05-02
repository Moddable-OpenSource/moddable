/*
 * Copyright (c) 2024 Moddable Tech, Inc.
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
class AudioIn @ "xs_audioin_destructor" {
	constructor(dictionary) @ "xs_audioin_constructor";
	close() @ "xs_audioin_close";
	read(samples) @ "xs_audioin_read";
	start() @ "xs_audioin_start";
	stop() @ "xs_audioin_stop";
	
	get format() @ "xs_audioin_get_format";
	set format(it) @ "xs_audioin_set_format";

	get bitsPerSample() @ "xs_audioin_get_bitsPerSample";
	get channels() @ "xs_audioin_get_numChannels";
	get sampleRate() @ "xs_audioin_get_sampleRate";
	get audioType() { return "LPCM" }
}

AudioIn.Async = class extends AudioIn {
	#queue = [];
	constructor(dictionary) {
		super({
			...dictionary,
			onReadable: (size) => {
				const queue = this.#queue;
				while ((queue.length > 0) && (size > 0)) {
					const item = queue[0];
					let delta = item.size - item.offset;
					if (delta > size)
						delta = size;
					const samples = new Uint8Array(item.buffer, item.offset, delta);
					super.read(samples);
					item.offset += delta;
					size -= delta;
					if (item.offset == item.size) {
						queue.shift();
						if (item.callback)
							item.callback.call(this, null, item.samples);
					}
				}
			},
		});
	}
	read(samples, callback) {
		let item;
		if (typeof samples === 'number')
			samples = new ArrayBuffer(samples);
		if (ArrayBuffer.isView(samples))
			item = { buffer:samples.buffer, offset:samples.byteOffset, size:samples.byteLength, callback, samples };
		else
			item = { buffer:samples, offset:0, size:samples.byteLength, callback, samples };
		this.#queue.push(item);
	}
}

export default AudioIn;
