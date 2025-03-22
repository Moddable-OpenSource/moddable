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

import Mixer from "pins/audioout";

let mixer = null;

class AudioOutMixer extends Mixer {
	constructor(dictionary) {
		super(dictionary);
		this.bufferSize = (((this.bitsPerSample * this.numChannels) >> 3) * this.sampleRate) >> 5; //??
		this.queueLength = Math.floor(this.length(0) / 3);
		this.tracks = [ null, null, null, null ];
		this.volumes = [ 256, 256, 256, 256 ];
		this.start();
	}	
	add(track) {
		const index = this.tracks.findIndex(item => item == null);	
		if (index < 0)
			throw new RangeError("too many tracks");
		this.tracks[index] = track;
		return index;
	}
	callback(index) {
		const track = this.tracks[index];
		if (track)
			track.callback();	
	}
	remove(index) {
		this.tracks[index] = null;
		index = this.tracks.findIndex(item => item != null);	
		if (index < 0) {
			this.close();
			mixer = null;
		}
	}
	getVolume(index) {
		return this.volumes[index] / 256;
	}
	setVolume(index, it) {
		this.volumes[index] = Math.round(it * 256);
	}
}

class AudioOutTrack {
	constructor(dictionary) {
		if (mixer == null)
			mixer = new AudioOutMixer(dictionary);
		this.index = mixer.add(this);
		this.running = false;
	}
	callback() {
		debugger
	}
	close() {
		mixer.enqueue(this.index, Mixer.Flush);
		mixer.remove(this);
	}
	start() {
		this.running = true;
	}
	stop() {
		this.running = false;
	}
	
	get format() {
		return "buffer";
	}
	set format(it) {
		if (it != "buffer")
			throw new RangeError("invalid format");
	}

	get bitsPerSample() {
		return mixer.bitsPerSample;
	}
	get channels() {
		return mixer.numChannels;
	}
	get sampleRate() {
		return mixer.sampleRate;
	}
	
	get volume() {
		return mixer.getVolume(this.index);
	}
	set volume(it) {
		mixer.setVolume(this.index, it);
	}
}

class AudioOut extends AudioOutTrack {
	constructor(dictionary) {
		super(dictionary);
		this.onWritable = dictionary.onWritable;
		this.queue = this.buildCircularQueue(this.index);
	}
	buildCircularQueue(index) {
		const result = {
			next:null,
		};
		let former = result, current;
		for (let i = 0; i < mixer.queueLength; i++) {
			former.next = current = {
				buffer: new SharedArrayBuffer(mixer.bufferSize),
				next:null,
				i
			};
			mixer.enqueue(index, Mixer.Volume, mixer.volumes[index]);
			mixer.enqueue(index, Mixer.RawSamples, current.buffer, 1, 0, mixer.bufferSize);
			mixer.enqueue(index, Mixer.Callback, index);
			former = current;
		}
		current.next = result.next;
		return result.next;
	}
	callback() {
		const current = this.queue;
		const index = this.index;
		if (current) {
			this.zero(current.buffer);
			if (this.running) {
				this.buffer = current.buffer;
				this.offset = 0;
				this.onWritable.call(this, mixer.bufferSize);
				this.buffer = null;
				this.offset = 0;
			}
			mixer.enqueue(index, Mixer.Volume, mixer.volumes[index]);
			mixer.enqueue(index, Mixer.RawSamples, current.buffer, 1, 0, mixer.bufferSize);
			mixer.enqueue(index, Mixer.Callback, index);
			this.queue = current.next;
		}
	}
	write(samples) @ "xs_audioout_write";
	zero(buffer) @ "xs_audioout_zero";
}

AudioOut.Async = class extends AudioOutTrack {
	constructor(dictionary) {
		super(dictionary);
		this.queue = [];
	}
	callback() {
		const queue = this.queue;
		const item = queue.shift();
		if (item.callback)
			item.callback.call(this);
	}
	write(samples, callback) {
		const index = this.index;
		this.queue.push({ callback });
		mixer.enqueue(index, Mixer.Volume, mixer.volumes[index]);
		if (ArrayBuffer.isView(samples))
			mixer.enqueue(index, Mixer.RawSamples, samples.buffer, 1, samples.byteOffset, samples.byteLength);
		else
			mixer.enqueue(index, Mixer.RawSamples, samples, 1, 0, samples.byteLength);
		mixer.enqueue(index, Mixer.Callback, index);
	}
}

export default AudioOut;
