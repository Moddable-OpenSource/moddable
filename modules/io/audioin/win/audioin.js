/*
 * Copyright (c) 2024-2025  Moddable Tech, Inc.
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
class AudioIn extends Native("xs_audioin_destructor") {
	constructor(dictionary) { super(); native("xs_audioin_constructor").call(this, dictionary); };
	close() { return native("xs_audioin_close").call(this); };
	read(samples) { return native("xs_audioin_read").call(this, samples); };
	start() { return native("xs_audioin_start").call(this); };
	stop() { return native("xs_audioin_stop").call(this); };
	
	get format() { return native("xs_audioin_get_format").call(this); };
	set format(it) { native("xs_audioin_set_format").call(this, it); };

	get bitsPerSample() { return native("xs_audioin_get_bitsPerSample").call(this); };
	get channels() { return native("xs_audioin_get_numChannels").call(this); };
	get sampleRate() { return native("xs_audioin_get_sampleRate").call(this); };
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
