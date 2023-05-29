/*
 * Copyright (c) 2022-2023  Moddable Tech, Inc.
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

import MP3 from "mp3/decode";

export default class {
	#audio;
	#stream;
	#http;
	#request;
	#playing = [];
	#free = [];
	#ready;		// undefined while initializing, false if not buffered / playing, true if buffers full to play / playing
	#samplesQueued = 0;
	#targetSamplesQueued = 1152 * 4;
	#callbacks = {};
	#pending = [];
	#readBuffer;
	#info = {};
	#mp3 = new MP3;

	constructor(options) {
//@@		const bufferDuration = options.bufferDuration ?? 1000;

		if (options.onPlayed)
			this.#callbacks.onPlayed = options.onPlayed;
		if (options.onReady)
			this.#callbacks.onReady = options.onReady;
		if (options.onError)
			this.#callbacks.onError = options.onError;
		if (options.onDone)
			this.#callbacks.onDone = options.onDone;

		const o = {
			...options.http,
			host: options.host
		};
		if (options.port)
			o.port = options.port; 
		this.#http = new options.http.io(o);
		this.#request = this.#http.request({
			...options.request,
			path: options.path,
			onHeaders: (status, headers) => {
				if (2 !== Math.idiv(status, 100)) {
					this.#callbacks.onError?.call(this, "http request failed, status " + status);
					this.#http.close();
					this.#http = this.#request = undefined;
					return;
				}
//@@ could grab content-length here
				
			},
			onReadable: (count) => {
				if (!this.#readBuffer) {
					this.#readBuffer = new Uint8Array(8192);			// must be able to hold a full mp3 frame + MP3.BUFFER_GUARD
					this.#readBuffer.position = 0;
				}

				this.#request.readable = count;
				this.#fillQueue();
			},
			onDone: () => {
				this.#info.done = 1;
			},
			onError: e => {
				this.#callbacks.onError?.call(this, e);
			}
		});

		const audio = options.audio.out;
		this.#audio = audio;
		this.#stream = options.audio.stream ?? 0;
		audio.callbacks ??= [];
		audio.callbacks[this.#stream] = samples => {
			if (!samples) {
				this.#callbacks.onDone?.call(this);
				return;
			}

			this.#samplesQueued -= samples;
			let played = this.#playing.shift();
			this.#free.push(played);
			this.#callbacks.onPlayed?.call(this, played);
			played = undefined;

			this.#fillQueue();

			if (0 === this.#samplesQueued) {
				this.#ready = false;
				this.#pending = [];

				if (this.#info.done) {
					if (1 === this.#info.done) {
						this.#info.done = 2;
						this.#callbacks.onDone?.call(this);
					}
				}
				else
					this.#callbacks.onReady?.call(this, false);
			}
		};
	}
	close() {
		if (this.#audio) {
			this.#audio.enqueue(this.#stream, this.#audio.constructor.Flush);
			this.#audio.callbacks[this.#stream] = null;
		}

		this.#mp3?.close();

		this.#http?.close();
		this.#http = this.#audio = this.#playing = this.#pending = this.#free = this.#mp3 = undefined;
	}
	#fillQueue() {
		const readBuffer = this.#readBuffer;
		do {
			let use = this.#request.readable;
			let available = readBuffer.length - readBuffer.position;
			if (use > available) use = available;;
			if (use) {
				this.#request.read(readBuffer.subarray(readBuffer.position, readBuffer.position + use));
				this.#request.readable -= use;
				readBuffer.position += use;
			}

			if (!readBuffer.position)
				break;		// no data to parse

			if (this.#samplesQueued >= this.#targetSamplesQueued)
				break;		// audio out queue full

			if (this.#audio.length(this.#stream) < 2)
				break;		// no free queue elements

			const found = MP3.scan(readBuffer, 0, readBuffer.position, this.#info);
			if (!found || ((found.position + found.length + MP3.BUFFER_GUARD) > readBuffer.position)) { 
				// partial frame, at best
				if (found) {
					readBuffer.copyWithin(0, found.position, readBuffer.position);
					readBuffer.position -= found.position;
				}
				else {
					use = 4;
					if (readBuffer.position < 4) use = readBuffer.position;
					readBuffer.copyWithin(0, readBuffer.position - use, readBuffer.position);
					readBuffer.position = use;
				}
				if (this.#request.readable)
					continue;
				break;
			}

			if (undefined === this.#ready) {
//				this.#targetSamplesQueued = Math.idiv((this.#info.sampleRate >> 1) + 1151, 1152);
				this.#ready = false;
			}

			const slice = this.#free.shift() ?? (new SharedArrayBuffer(1152 * 2));
			const byteLength = this.#mp3.decode(readBuffer.subarray(found.position, found.position + found.length + MP3.BUFFER_GUARD), slice);
			if (byteLength) {
				if (this.#pending)
					this.#pending.push(slice);
				else {
					this.#audio.enqueue(this.#stream, this.#audio.constructor.RawSamples, slice, 1, 0, slice.samples);
					this.#audio.enqueue(this.#stream, this.#audio.constructor.Callback, slice.samples);
					this.#playing.push(slice);
				}

				this.#samplesQueued += slice.samples;
			}

			readBuffer.copyWithin(0, found.position + byteLength, readBuffer.position);
			readBuffer.position -= (found.position + byteLength);
		} while (true);

		if (!this.#ready && (this.#samplesQueued >= this.#targetSamplesQueued)) {
			this.#ready = true;

			while (this.#pending.length) {
				const slice = this.#pending.shift();
				this.#audio.enqueue(this.#stream, this.#audio.constructor.RawSamples, slice, 1, 0, slice.samples);
				this.#audio.enqueue(this.#stream, this.#audio.constructor.Callback, slice.samples);
				this.#playing.push(slice);
			}
			this.#pending = undefined;

			this.#callbacks.onReady?.call(this, true);
		}
	}
}
