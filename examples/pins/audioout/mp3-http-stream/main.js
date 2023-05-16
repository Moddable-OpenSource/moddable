/*
 * Copyright (c) 2022-2023  Moddable Tech, Inc.
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
import MP3Streamer from "mp3streamer";

function calculatePower(samplea) @ "xs_calculatePower";

const audio = new AudioOut({
	sampleRate: 44100,
	streams: 1
});

// audio.enqueue(0, AudioOut.Volume, 64);

new MP3Streamer({
	http: device.network.http,
	host: "ice4.somafm.com",
	path: "/indiepop-128-mp3",
	audio: {
		out: audio,
		stream: 0
	},
	onPlayed(buffer) {
		const power = calculatePower(buffer);
		trace(`MP3 power ${Math.round(power)}\n`);
	},
	onReady(state) {
		trace(`MP3 Ready: ${state}\n`);
		if (state)
			audio.start();
		else
			audio.stop();
	},
	onError(e) {
		trace("MP3 ERROR: ", e, "\n");
	},
	onDone() {
		trace("MP3 Done\n");
	}
});

