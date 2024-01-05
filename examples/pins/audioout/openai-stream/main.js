/*
 * Copyright (c) 2024  Moddable Tech, Inc.
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
import OpenAIStreamer from "openaistreamer";

const key = "PUT-YOUR-API-KEY-HERE";
if (key.includes("API-KEY"))
	throw new Error("Set your OpenAI API key");		// https://platform.openai.com/docs/guides/production-best-practices/api-keys

const audio = new AudioOut({});
audio.start();

new OpenAIStreamer({
	key,
	model: "tts-1",
	voice: "alloy",
	input: "The Moddable SDK is a combination of development tools and runtime software to create applications for microcontrollers.",
	audio: {
		out: audio,
		stream: 0
	},
	onError(e) {
		trace("OpenAI ERROR: ", e, "\n");
	},
	onDone() {
		trace("OpenAI Done\n");
		this.close();
	}
})
