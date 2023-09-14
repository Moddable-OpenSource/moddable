/*
 * Copyright (c) 2023  Moddable Tech, Inc.
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
import ElevenLabsStreamer from "elevenlabsstreamer";

const key = "PUT-YOUR-API-KEY-HERE";
if (key.includes("API-KEY"))
	throw new Error("Set your ElevenLabs API key");		// https://docs.elevenlabs.io/authentication/01-xi-api-key

const audio = new AudioOut({});
audio.start();

new ElevenLabsStreamer({
	key,
	voice: "AZnzlk1XvdvUeBnXmlld",
	latency: 2,
	text: "The Moddable SDK is a combination of development tools and runtime software to create applications for microcontrollers.",
	audio: {
		out: audio,
		stream: 0
	},
	onError(e) {
		trace("ElevenLabs ERROR: ", e, "\n");
	},
	onDone() {
		trace("ElevenLabs Done\n");
		this.close();
	}
})
