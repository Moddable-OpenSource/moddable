/*
 * Copyright (c) 2020-2023  Moddable Tech, Inc.
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

const kMAUDHeader = 12;

class SBCStreamer {
	#http;
	#request;
	#audio;
	#stream;
	#next;
	#playing = [];
	#free = [];
	#pending = [];
	#ready;		// undefined while initializing, false if not buffered / playing, true if buffers full to play / playing
	#header;
	#bytesPerBlock;
	#bytesQueued = 0;
	#targetBytesQueued;
	#callbacks = {};
	#bytes;

	constructor(options) {
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
		this.#targetBytesQueued = options.bufferDuration ?? 1000;
		this.#http = new options.http.io(o);
		this.#request = this.#http.request({
			path: options.path,
			onHeaders: (status, headers) => {
				if (2 !== Math.idiv(status, 100)) {
					this.#callbacks.onError?.("http request failed, status " + status);
					this.#http.close();
					this.#http = this.#request = undefined;
					return;
				}

				const length = headers.get("content-length");
				this.#bytes = (undefined === length) ? Infinity : parseInt(length);
			},
			onReadable: (count) => {
				if (undefined === this.#ready) {
					if (count < 4)
						return;

					const bytes = new Uint8Array(this.#request.read(4));
					count -= 4;
					this.#header = SBCStreamer.parseHeader(bytes);
					if (!this.#header) {
						this.#callbacks.onError?.("invalid SBC header");
						this.#http.close();
						this.#http = this.#request = undefined;
						return;
					}

					this.#targetBytesQueued = Math.idiv(this.#header.sampleRate * (this.#targetBytesQueued / 1000), 128) * this.#header.bytesPerChunk;		// bytes in bufferDuration (128 samples per chunk)
					this.#bytesPerBlock = Math.idiv(this.#targetBytesQueued, this.#header.bytesPerChunk << 3) * this.#header.bytesPerChunk;					// 1/8th second blocks

					this.#ready = false;	

					this.#next = new Uint8Array(new SharedArrayBuffer(kMAUDHeader + this.#bytesPerBlock));

					this.#next.set(bytes, kMAUDHeader);
					this.#next.position = kMAUDHeader + 4;
				}
				this.#request.readable = count;
				this.#fillQueue();
			},
			onDone: () => {
				this.#audio.enqueue(this.#stream, this.#audio.constructor.Callback, 0);
			},
			onError: e => {
				this.#callbacks.onError?.(e);
			}
		});

		const audio = options.audio.out;
		this.#audio = audio;
		this.#stream = options.audio.stream ?? 0;
		audio.callbacks ??= [];
		audio.callbacks[this.#stream] = bytes => {
			if (!bytes) {
				this.#callbacks.onDone?.();
				return;
			}

			this.#bytesQueued -= bytes;
			let played = this.#playing.shift();
			this.#callbacks.onPlayed?.(played);
			if (played.byteLength === (kMAUDHeader + this.#bytesPerBlock)) {
				played.position = kMAUDHeader;
				this.#free.push(played);
			}
			played = undefined;

			this.#fillQueue();
			if (0 === this.#bytesQueued) {
				this.#ready = false;
				this.#pending = [];
				this.#callbacks.onReady?.(false);
			}
		};
	}
	close() {		
		this.#audio.enqueue(this.#stream, this.#audio.constructor.Flush);
		this.#audio.callbacks[this.#stream] = null;

		this.#http.close();
		this.#http = this.#audio = this.#playing = this.#pending = this.#free = undefined;
	}

	#fillQueue() {
		while ((this.#bytesQueued < this.#targetBytesQueued) &&
				this.#request.readable &&
				(this.#audio.length(this.#stream) >= 2)) {
			let next = this.#next;
			if (!next) {
				this.#next = next = this.#free.shift();
				if (!next)
					this.#next = next = new Uint8Array(new SharedArrayBuffer(kMAUDHeader + this.#bytesPerBlock));
				next.position = kMAUDHeader;
			}

			const use = Math.min(next.byteLength - next.position, this.#request.readable);
			this.#request.read(next.subarray(next.position, next.position + use));
			this.#request.readable -= use;
			next.position += use;
			this.#bytes -= use;
			if ((next.position === next.byteLength) || !this.#bytes) {
				if (this.#pending)
					this.#pending.push(next);
				else
					this.#enqueue(next);
				this.#bytesQueued += (next.position - kMAUDHeader);
				this.#next = undefined;
			}
		}

		if (!this.#ready && (this.#bytesQueued >= this.#targetBytesQueued)) {
			this.#ready = true;

			while (this.#pending.length)
				this.#enqueue(this.#pending.shift());
			this.#pending = undefined;

			this.#callbacks.onReady?.(true);
		}
	}
	#enqueue(next) {
		const samples = 128 * Math.idiv(next.position - kMAUDHeader, this.#header.bytesPerChunk);
		next[0] = 0x6D;
		next[1] = 0x61;
		next[2] = 1;
		next[3] = 8;	// bits per sample
		next[4] = this.#header.sampleRate & 0xff;
		next[5] = this.#header.sampleRate >> 8;
		next[6] = this.#header.channels;
		next[7] = 2;	// format: SBC
		next[8] = samples & 0xff;
		next[9] = (samples >> 8) & 0xff;
		next[10] = (samples >> 16) & 0xff;
		next[11] = (samples >> 24) & 0xff;

		this.#audio.enqueue(this.#stream, this.#audio.constructor.Samples, next);
		this.#audio.enqueue(this.#stream, this.#audio.constructor.Callback, next.position - kMAUDHeader);
		this.#playing.push(next);
	}

	static parseHeader(header) {
		if (0x9C !== header[0])
			return;

		// https://github.com/rossumur/espflix/blob/master/src/sbc_decoder.cpp
		const frequency = (header[1] >> 6) & 0x03;
		const blocks = [4, 8, 12, 16][(header[1] >> 4) & 0x03];
		const mode = (header[1] >> 2) & 0x03;
		// const allocation = (header[1] >> 1) & 0x01;
		const subbands = (header[1] & 0x01) ? 8 : 4;

		// https://android.googlesource.com/platform/system/bt/+/master/embdrv/sbc/decoder/srce/bitalloc.c
		let bytesPerChunk = blocks * header[2];
		if (0 !== mode)
			return;		// mono only
		bytesPerChunk += subbands << 2;
		bytesPerChunk = 4 + ((bytesPerChunk + 7) >> 3);

		return {
			sampleRate: [16000, 32000, 44100, 48000][frequency],
			channels: mode ? 2 : 1,
			bytesPerChunk
		};
	}
}

export default SBCStreamer;
