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

import AudioOut from "embedded:io/audio/out";
import {MAUD, SampleFormat} from "maudHeader";
import Resource from "Resource";

const one = loadSound("one-voice.maud");
const two = loadSound("two-tone.maud");
let playing = one;

const output = new AudioOut({
	onWritable(size) {
		do {
			let use = playing.byteLength - playing.position;
			if (use > size) use = size;
			this.write(playing.subarray(playing.position, playing.position + use));
			playing.position += use;
			if (playing.position === playing.byteLength) {
				playing.position = 0;
				playing = (playing === one) ? two : one;
			}
			size -= use;
		} while (size);
	}
});
output.start();

function loadSound(name) {
	let snd = new MAUD(new Resource(name));
	if (!(1 === snd.version) && ("ma" === snd.tag) && (SampleFormat.Uncompressed === snd.sampleFormat) && (16 === snd.bitPerSample)  && (1 === snd.channels))
		throw new Error;
	snd = new Uint8Array(snd.buffer, snd.byteOffset + snd.byteLength, snd.bufferSamples * 2);
	snd.position = 0;
	
	return snd;
}
