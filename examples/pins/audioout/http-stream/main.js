/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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

import AudioOut from "pins/audioout"
import WavStreamer from "wavstreamer";
import SBCStreamer from "sbcstreamer";

function calculatePower(samplea) @ "xs_calculatePower";

const audio = new AudioOut({});

function streamWav() {
	new WavStreamer({
		http: device.network.http,
		host: "test.moddable.com",
		path: "/audio/ChristmasMusic/jingle.wav",
		audio: {
			out: audio,
			stream: 0
		},
		onPlayed(buffer) {
			const power = calculatePower(buffer);
			trace(`WAV power ${Math.round(power)}\n`);
		},
		onReady(state) {
			trace(`WAV Ready: ${state}\n`);
			if (state)
				audio.start();
			else
				audio.stop();
		},
		onError(e) {
			trace("WAV ERROR: ", e, "\n");
		},
		onDone() {
			trace("WAV Done\n");
			streamSBC();
		}
	});
}

function streamSBC() {
	new SBCStreamer({
		http: device.network.http,
		host: "test.moddable.com",
		path: "/audio/ChristmasMusic/jesu.sbc",
		audio: {
			out: audio,
			stream: 0
		},
		onReady(state) {
			trace(`SBC Ready: ${state}\n`);
			if (state)
				audio.start();
			else
				audio.stop();
		},
		onError(e) {
			trace("SBC ERROR: ", e, "\n");
		},
		onDone() {
			trace("SBC Done\n");
			streamWav();
		}
	});
}

streamWav();
