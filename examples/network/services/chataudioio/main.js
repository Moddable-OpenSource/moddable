/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

import ChatAudioIO from "ChatAudioIO";

const states = [
	"DISCONNECTED",
	"DISCONNECTING",
	"SPEAKING",
	"CONNECTING",
	"LISTENING",
	"WAITING"
];
states[-1] = "FAILED";

const chat = new ChatAudioIO({
	specifier: "humeAIEVI",
	voiceName: "Sunny",
	instructions: "You're a hostile firsherman with a salty sense of humor. You dislike people and care even less for fish.",
	onStateChanged(state) {
		trace(`State: ${states[state]} ${this.error ?? ""}\n`);
	},
	onInputTranscript(text) {
		trace(`User: ${text}\n`);
	},
	onOutputTranscript(text) {
		trace(`Agent: ${text}\n`);
	},
});
chat.connect();
