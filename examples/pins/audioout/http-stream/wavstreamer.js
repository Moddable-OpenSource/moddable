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
 
 /*
	Streaming from http
	
		uses Ecma-419 HTTPClient implementation (much better for streaming)
		collects received audio into fixed size block (bytesPerBlock)
		tries to top-up queue after audio buffer played and when new network data received
		audio queue target defaults to one second of audio
		down-mixes stereo to mono
*/

/*
	ffmpeg -i bflatmajor.wav -listen 1 -content_type "audio/L16;rate=11025&channels=2" -f s16be -ar 11025 -acodec pcm_s16be http://127.0.0.1:8080
*/

import WavReader from "data/wavreader";

class WavStreamer {
	#audio;
	#stream;
	#http;
	#request;
	#playing = [];
	#free = [];
	#ready;		// undefined while initializing, false if not buffered / playing, true if buffers full to play / playing
	#next;
	#bytes = Infinity;		// remaining in stream
	#bytesQueued = 0;
	#targetBytesQueued;
	#bytesPerSample = 2;
	#bytesPerBlock;
	#callbacks = {};
	#pending = [];
	#swap;		// endian swap
	#channels;	// 1 or 2

	constructor(options) {
		const waveHeaderBytes = options.waveHeaderBytes ?? 512;
		const bufferDuration = options.bufferDuration ?? 1000;

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
					this.#callbacks.onError?.("http request failed, status " + status);
					this.#http.close();
					this.#http = this.#request = undefined;
					return;
				}
				
				let mime = headers.get("content-type");
				if (mime?.toLowerCase().startsWith("audio/l16;")) {
					let channels = 1, rate;
					mime = mime.substring(10).split("&");
					for (let i = 0; i < mime.length; i++) {
						const part = mime[i];
						if (part.startsWith("rate="))
							rate = parseInt(part.substring(5));
						else if (part.startsWith("channels="))
							channels = parseInt(part.substring(9));
					}
					if (this.#audio.sampleRate !== rate) {
						this.#callbacks.onError?.("invalid audio/L16");
						this.#http.close();
						this.#http = this.#request = undefined;
						return;
					}

					let length = headers.get("content-length");
					if (length)
						this.#bytes = parseInt(length);

					this.#ready = false;	
					this.#swap = WavStreamer.swap;
					this.#channels = channels;

					this.#bytesPerSample = channels * 2; 
					this.#targetBytesQueued = this.#audio.sampleRate * 2 * (bufferDuration / 1000);		// always mono because of downmixing
					this.#bytesPerBlock = Math.idiv(this.#targetBytesQueued, 8);
					this.#bytesPerBlock -= Math.imod(this.#bytesPerBlock, this.#bytesPerSample); 
				}
			},
			onReadable: (count) => {
				this.#request.readable = count;
				if (undefined === this.#ready) {
					if (count < waveHeaderBytes)
						return;

					const buffer = this.#request.read(waveHeaderBytes);
					const wav = new WavReader(buffer);
					let error;
					if (1 !== wav.audioFormat)
						error = "format";
					else if (this.#audio.sampleRate !== wav.sampleRate)
						error = "sampleRate";
					else if (this.#audio.bitsPerSample !== wav.bitsPerSample)
						error = "bitsPerSample";
					else if ((1 !== wav.numChannels) && (2 !== wav.numChannels))
						error = "channels";
					if (error) {
						this.#callbacks.onError?.("invalid WAV: " + error);
						this.#http.close();
						this.#http = this.#request = undefined;
						return;
					}

					this.#channels = wav.numChannels;
					this.#ready = false;	

					this.#bytesPerSample = (wav.numChannels * this.#audio.bitsPerSample) >> 3; 
					this.#targetBytesQueued = this.#audio.sampleRate * (this.#audio.bitsPerSample >> 3) * (bufferDuration / 1000);		// always mono because of downmixing
					this.#bytesPerBlock = Math.idiv(this.#targetBytesQueued, 8);
					this.#bytesPerBlock -= Math.imod(this.#bytesPerBlock, this.#bytesPerSample); 

					this.#request.readable -= waveHeaderBytes;
					this.#next = new Uint8Array(new SharedArrayBuffer(this.#bytesPerBlock * wav.numChannels));
					this.#next.set(new Uint8Array(buffer, wav.position));
					this.#next.position = waveHeaderBytes - wav.position;

					this.#bytes = (wav.samples << 1) - this.#next.position;
				}
				this.#fillQueue();
			},
			onDone: () => {
				this.#targetBytesQueued = this.#bytesPerBlock;
				this.#fillQueue();
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
			if (played.byteLength === (this.#bytesPerBlock * this.#channels)) {
				played.position = 0;
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
		
		this.#http?.close();
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
					this.#next = next = new Uint8Array(new SharedArrayBuffer(this.#bytesPerBlock * this.#channels));
				next.position = 0;
			}

			const use = Math.min(next.byteLength - next.position, this.#request.readable);
			this.#request.read(next.subarray(next.position, next.position + use));
			this.#request.readable -= use;
			next.position += use;
			this.#bytes -= use;
			if ((next.position === next.byteLength) || !this.#bytes) {
				this.#swap?.(next);
				if (2 === this.#channels) {
					next = new Uint8Array(new SharedArrayBuffer(next.position >> 1));
					WavStreamer.mix(this.#next, next);
					next.position = this.#next.position >> 1;
					this.#next = next;
				}
				if (this.#pending)
					this.#pending.push(next);
				else {
					this.#audio.enqueue(this.#stream, this.#audio.constructor.RawSamples, next, 1, 0, next.position / (this.#bytesPerSample / this.#channels));	// mono because of downmixing
					this.#audio.enqueue(this.#stream, this.#audio.constructor.Callback, next.position);
					this.#playing.push(next);
				}
				this.#bytesQueued += next.position;
				this.#next = undefined;
			}
		}

		if (!this.#ready && (this.#bytesQueued >= this.#targetBytesQueued)) {
			this.#ready = true;

			while (this.#pending.length) {
				const next = this.#pending.shift();
				this.#audio.enqueue(this.#stream, this.#audio.constructor.RawSamples, next, 1, 0, next.position / (this.#bytesPerSample / this.#channels));		// mono because of downmixing
				this.#audio.enqueue(this.#stream, this.#audio.constructor.Callback, next.position);
				this.#playing.push(next);
			}
			this.#pending = undefined;

			this.#callbacks.onReady?.(true);
		}
	}
	
	static swap(buffer) @ "xs_wavestream_swap";
	static mix(from, to) @ "xs_wavestream_mix";
}

export default WavStreamer;
