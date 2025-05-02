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

class AudioOut @ "xs_audioout_destructor_" {
	constructor(options) @ "xs_audioout_constructor_";
	close() @ "xs_audioout_close_";
	start() @ "xs_audioout_start_";
	stop() @ "xs_audioout_stop_";
	write(samples) @ "xs_audioout_write_";
	
	get format() @ "xs_audioout_get_format_";
	set format(it) @ "xs_audioout_set_format_";

	get bitsPerSample() @ "xs_audioout_get_bitsPerSample_";
	get channels() @ "xs_audioout_get_numChannels_";
	get sampleRate() @ "xs_audioout_get_sampleRate_";
	get audioType() {return "LPCM"}
	
	get volume() @ "xs_audioout_get_volume_";
	set volume(it) @ "xs_audioout_set_volume_";
}

AudioOut.Async = class extends AudioOut {
	#queue = [];
	constructor(options) {
		super({
			...options,
			onWritable: (size) => {
				const queue = this.#queue;
				while ((queue.length > 0) && (size > 0)) {
					const item = queue[0];
					let delta = item.size - item.offset;
					if (delta > size)
						delta = size;
					const samples = new Uint8Array(item.buffer, item.offset, delta);
					super.write(samples);
					item.offset += delta;
					size -= delta;
					if (item.offset == item.size) {
						queue.shift();
						if (item.callback)
							item.callback.call(this, null);
					}
				}
			},
		});
	}
	write(samples, callback) {
		let item;
		if (ArrayBuffer.isView(samples))
			item = { buffer:samples.buffer, offset:samples.byteOffset, size:samples.byteLength, callback }
		else
			item = { buffer:samples, offset:0, size:samples.byteLength, callback }
		this.#queue.push(item);
	}
}

export default AudioOut;
