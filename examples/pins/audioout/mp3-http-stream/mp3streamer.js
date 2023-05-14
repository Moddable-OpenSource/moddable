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
	#next;
	#framesQueued = 0;
	#targetFramesQueued = 1;
	#callbacks = {};
	#pending = [];
	#readBuffer = new Uint8Array(1024);
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
				this.#request.readable = count;
				this.#fillQueue();
			},
			onDone: () => {
				this.#fillQueue();
				this.#audio.enqueue(this.#stream, this.#audio.constructor.Callback, 0);
			},
			onError: e => {
				this.#callbacks.onError?.call(this, e);
			}
		});
		
		this.#readBuffer.position = 0;

		const audio = options.audio.out;
		this.#audio = audio;
		this.#stream = options.audio.stream ?? 0;
		audio.callbacks ??= [];
		audio.callbacks[this.#stream] = bytes => {
			if (!bytes) {
				this.#callbacks.onDone?.call(this);
				return;
			}

			this.#framesQueued -= 1;
			let played = this.#playing.shift();
			this.#free.push(played);
			this.#callbacks.onPlayed?.call(this, played);
			played = undefined;

			this.#fillQueue();

			if (0 === this.#framesQueued) {
				this.#ready = false;
				this.#pending = [];
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
		while ((this.#framesQueued < this.#targetFramesQueued) &&
				(this.#request.readable || readBuffer.position) &&
				(this.#audio.length(this.#stream) >= 2)) {
			let use = this.#request.readable;
			let available = readBuffer.length - readBuffer.position;
			if (use > available) use = available;;
			if (use) {
				this.#request.read(readBuffer.subarray(readBuffer.position, readBuffer.position + use));
				this.#request.readable -= use;
				readBuffer.position += use;
			}

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
				this.#targetFramesQueued = Math.idiv((this.#info.sampleRate >> 1) + 1151, 1152);		//@@ half second
				this.#ready = false;
			}

			const slice = this.#free.shift() ?? (new SharedArrayBuffer(1152 * 2));
			let result = this.#mp3.decode(readBuffer.subarray(found.position, found.position + found.length + MP3.BUFFER_GUARD), slice);
			if (!result)
				throw new Error("bad mp3 data");

			if (this.#pending)
				this.#pending.push(slice);
			else {
				this.#audio.enqueue(this.#stream, this.#audio.constructor.RawSamples, slice, 1, 0, slice.byteLength);
				this.#audio.enqueue(this.#stream, this.#audio.constructor.Callback, 1);
				this.#playing.push(slice);
			}

			this.#framesQueued += 1;

			readBuffer.copyWithin(0, found.position + found.length, readBuffer.position);
			readBuffer.position -= (found.position + found.length);
		}

		if (!this.#ready && (this.#framesQueued >= this.#targetFramesQueued)) {
			this.#ready = true;

			while (this.#pending.length) {
				const slice = this.#pending.shift();
				this.#audio.enqueue(this.#stream, this.#audio.constructor.RawSamples, slice, 1, 0, slice.byteLength);
				this.#audio.enqueue(this.#stream, this.#audio.constructor.Callback, 1);
				this.#playing.push(slice);
			}
			this.#pending = undefined;

			this.#callbacks.onReady?.call(this, true);
		}
	}
}
