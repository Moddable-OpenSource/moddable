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
import ResourceStreamer from "mp3resourcestreamer";
import Timer from "timer";


/*

	Let's make some test files!

		https://trac.ffmpeg.org/wiki/Encode/MP3

	ffmpeg -i input.wav -codec:a libmp3lame -qscale:a 2 output.mp3

	curl --url http://ice2.somafm.com/groovesalad-128-mp3 --output groovesalad.mp3

*/

function calculatePower(samplea) @ "xs_calculatePower";

const audio = new AudioOut({
	sampleRate: 44100
});
let streamer

async function stream() {
	return new Promise((resolve, reject) => {
		if (streamer != null) {
			reject(new Error("already playing"));
			return
		}
		streamer = new ResourceStreamer({
		path: "startup.mp3",
		audio: {
			out: audio,
			stream: 0,
			sampleRate: 48000
		},
		onPlayed(buffer) {
			const power = calculatePower(buffer);
			trace("power " + Math.round(power) + "\n");
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
		audio.stop();
		trace(`play: ${count++}\n`)
		let willPlay = stream();
		let willFail = stream().catch(e => {trace(`error: ${e.message}\n`)}) // error: already playing
		await Promise.allSettled([willPlay, willFail])
		Timer.delay(2000)
	}
})()
