/*
 * Copyright (c) 2024 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import AudioIn from "embedded:io/audio/in";
import AudioOut from "embedded:io/audio/out";

const samples = [];
samples.total = 0;

trace("recording\n")

const input = new AudioIn({
	channels: 1,
	onReadable(size) {
		const buffer = new SharedArrayBuffer(size);
		input.read(buffer);
		const data = new Uint8Array(buffer);

		samples.push(data);
		samples.total += data.byteLength;

		if (samples.total >= input.sampleRate * (input.bitsPerSample / 8) * 5) {
			this.close();
			play();
		}
	}
});
input.start();
const {channels, sampleRate, bitsPerSample} = input;

function play() {
	samples.forEach(buffer => buffer.position = 0);

	trace("playing\n")
	const output = new AudioOut({
		sampleRate,
		channels,
		bitsPerSample,
		onWritable(size) {
			do {
				const playing = samples[0];
				let use = playing.byteLength - playing.position;
				if (use > size) use = size;
				output.write(playing.subarray(playing.position, playing.position + use));
				playing.position += use;
				if (playing.position === playing.byteLength) {
					samples.shift();
					if (0 === samples.length) {
						trace("done\n")
						return void this.close();
					}
				}
				size -= use;
			} while (size);
		}
	});
	output.start();
}
