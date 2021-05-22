/*
 * Copyright (c) 2018  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import AudioIn from "audioin"
import Timer from "timer"

let input = new AudioIn;
const readingsPerSecond = 8;
const sampleCount = Math.floor(input.sampleRate / readingsPerSecond);
if (16 !== input.bitsPerSample)
	throw new Error("16 bit samples only");

Timer.repeat(() => {
	const samples = new Int16Array(input.read(sampleCount));

	let total = 0;
	for (let i = 0; i < sampleCount; i++) {
		const sample = samples[i];
		if (sample < 0)
			total -= sample;
		else
			total += sample;
	}

	trace(`Average ${(total / sampleCount) | 0}\n`);
}, 1000 / readingsPerSecond);
