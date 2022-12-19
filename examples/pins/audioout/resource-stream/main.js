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
import ResourceStreamer from "resourcestreamer";
import Timer from "timer";

function calculatePower(samplea) @ "xs_calculatePower";

const audio = new AudioOut({});
let streamer

async function stream() {
	return new Promise((resolve, reject) => {
		if (streamer != null) {
			reject(new Error("already playing"));
			return
		}
		streamer = new ResourceStreamer({
		path: "bflatmajor.maud",
		audio: {
			out: audio,
			stream: 0,
			sampleRate: 11025
		},
		onPlayed(buffer) {
			const power = calculatePower(buffer);
			trace("power " + Math.round(power) + "\n");
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
			streamer = null
			reject(new Error("unknown error occured"))
		},
		onDone() {
			trace("DONE\n");
			streamer.close()
			streamer = null
			resolve()
		}
	});
})
}

(async function main() {
	let count = 0
	while (true){
		trace(`play: ${count++}\n`)
		let willPlay = stream();
		let willFail = stream().catch(e => {trace(`error: ${e.message}\n`)}) // error: already playing
		await Promise.allSettled([willPlay, willFail])
		Timer.delay(2000)
	}
})()
