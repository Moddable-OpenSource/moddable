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

function calculatePower(samplea) @ "xs_calculatePower";

const audio = new AudioOut({});

new WavStreamer({
	http: device.network.http,
	host: "localhost",
	path: "/example_16bit_mono_11025hz.wav",
	audio: {
		out: audio,
		stream: 0,
		sampleRate: 11025
	},
	onPlayed(buffer) {
		const power = calculatePower(buffer);
		trace(`power ${Math.round(power)}\n`);
	},
	onReady(state) {
		trace(`Ready: ${state}\n`);
		if (state)
			audio.start();
		else
			audio.stop();
	},
	onError(e) {
		trace("ERROR: ", e, "\n");
	},
	onDone() {
		trace("DONE\n");
	}
});
