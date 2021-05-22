/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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

import MY92x1 from "my92x1";
import Timer from "timer";

const MorseCode = {
	A: ".-",
	B: "-...",
	C: "-.-.",
	D: "-..",
	E: ".",
	F: "..-.",
	G: "--.",
	H: "....",
	I: "..",
	J: ".---",
	K: "-.-",
	L: ".-..",
	M: "--",
	N: "-.",
	O: "---",
	P: ".--.",
	Q: "--.-",
	R: ".-.",
	S: "...",
	T: "-",
	U: "..-",
	V: "...-",
	W: ".--",
	X: "-..-",
	Y: "-.--",
	Z: "--..",

	1: ".----",
	2: "..---",
	3: "...--",
	4: "----.",
	5: ".....",
	6: "-....",
	7: "--...",
	8: "---..",
	9: "----.",
	0: "-----",
}

const message = "SOS Maker Faire ".toUpperCase();

let encoded = new Int8Array(1024);
let encodedLength = 0;

for (let i = 0; i < message.length; i++) {
	let character = message[i];
	if (" " == character) {
		encoded[encodedLength - 1] = -7;	// space between words is 7
		continue;
	}

	let dotsAndDashes = MorseCode[message[i]];
	if (undefined == dotsAndDashes)
		continue;

	for (let j = 0; j < dotsAndDashes.length; j++) {
		if ('.' == dotsAndDashes[j])
			encoded[encodedLength++] = 1;	// dot
		else
			encoded[encodedLength++] = 3;	// dash

		encoded[encodedLength++] = -1;	// space between parts of letters is 1
	}

	encoded[encodedLength - 1] = -3;	// space between letters is 3
}

const light = new MY92x1;
const brightness = 16;
const interval = 125;

let position = -1;
let value = 0;
Timer.repeat(() => {
	if (0 == value) {
		position += 1;
		if (position == encodedLength)
			position = 0;
		value = encoded[position];
	}

	if (value < 0) {
		light.write(0, 0, 0, 0, 0);
		value += 1;
	}
	else if (value > 0) {
		light.write(brightness, brightness, 0, 0, 0, 0); // W C 0 G R B
		value -= 1;
	}
}, interval);
