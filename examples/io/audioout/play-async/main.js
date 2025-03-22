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

const one = loadSound("one.maud");
const two = loadSound("two.maud");

const output = new AudioOut.Async({});
output.start();

const writeOne = () => output.write(one, writeOne);
const writeTwo = () => output.write(two, writeTwo);

writeOne();
writeTwo();

function loadSound(name) {
	let snd = new MAUD(new Resource(name));
	if (!(1 === snd.version) && ("ma" === snd.tag) && (SampleFormat.Uncompressed === snd.sampleFormat) && (16 === snd.bitPerSample)  && (1 === snd.channels))
		throw new Error;
	snd = new Uint8Array(snd.buffer, snd.byteOffset + snd.byteLength, snd.bufferSamples * 2);
	snd.position = 0;
	
	return snd;
}
