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
 
import AudioOut from "pins/audioout";
import Resource from "Resource";

export default class Sound {
	static private = { audioOut:null, callbacks:null, streams:0, volume:256 };

	static callback(index) {
		Sound.private.callbacks[index].call();
	}
	static close() {
		let audioOut = this.private.audioOut
		if (audioOut) {
			audioOut.stop();
			audioOut.close();
			this.private.audioOut = null;
			this.private.callbacks = null;
			this.private.streams = 0;
		}
	}
	static open(streams = 1) {
		let audioOut = this.private.audioOut = new AudioOut({
			bitsPerSample:this.bitsPerSample, 
			numChannels:this.numChannels, 
			sampleRate:this.sampleRate, 
			streams
		});
		this.private.callbacks = new Array(streams);
		this.private.streams = streams;
		audioOut.callback = this.callback;
		audioOut.start();
		return audioOut;
	}
	static get bitsPerSample() @ "PiuSound_get_bitsPerSample";
	static get numChannels() @ "PiuSound_get_numChannels";
	static get sampleRate() @ "PiuSound_get_sampleRate";
	static get volume() {
		return this.private.volume / 256;
	}
	static set volume(it)  {
		this.private.volume = Math.round(it * 256);
	}
	constructor(it) {
		if (Array.isArray(it)) {
			this.tones = it;
			return;
		}

		let archive = it.archive;
		let path = it.path;
		if (!path)
			throw new URIError("Sound: no path!");
		if (!path.endsWith(".wav"))
			throw new URIError("Sound: not .wav!");
		path = path.slice(0, -4) + ".maud";
		if (!Resource.exists(path, archive))
			throw new URIError("Sound: " + it.path + " not found!");
		this.buffer = new Resource(path, archive);
		this.offset = it.offset ?? 0;
		this.size = it.size ?? -1;
	}
	play(stream = 0, repeat = 1, callback) {
		const audioOut = Sound.private.audioOut ?? Sound.open();
		audioOut.enqueue(stream, AudioOut.Flush);
		audioOut.enqueue(stream, AudioOut.Volume, Sound.private.volume);

		if (this.tones) {
			if (1 !== repeat)
				throw new Error("Sound: no repeat!");

			const tones = this.tones;
			let length = audioOut.length(stream) - (callback ? 1 : 0);
			if (length > tones.length)
				length = tones.length;

			for (let i = 0; i < length; i++)
				audioOut.enqueue(stream, AudioOut.Tone, tones[i].frequency, tones[i].samples ?? Infinity);
		}
		else {
			audioOut.enqueue(stream, AudioOut.Samples, this.buffer, repeat, this.offset, this.size);
		}

		Sound.private.callbacks[stream] = callback;
		if (callback)
			audioOut.enqueue(stream, AudioOut.Callback, stream);
	}
}
