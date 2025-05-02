/*
 * Copyright (c) 2024-2025 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

class ChatWorker {
	constructor(options) {
		self.onmessage = message => {
			const method = this[message.id];
			if (method)
				method.call(this, message);
		}
		const inputSampleRate = options.inputSampleRate ?? 24000;
		const outputSampleRate = options.outputSampleRate ?? 24000;
		self.postMessage({ id:"configureAudio", inputSampleRate, outputSampleRate });
	}
	post(id, param) {
		self.postMessage({ id, ...param });
	}
	configure(message) {
		this.configuration = message.configuration;
	}
	connect(message) {
		this.inputBuffer = message.inputBuffer;
		this.outputBuffer = message.outputBuffer;
	}
	disconnect() {
	}
	listened(message) {
	}
	postMessage(message) {
		self.postMessage(message);
	}
	sendAudio(message) {
		debugger
	}
	sendFunctionResult(message) {
		debugger
	}
	sendText(message) {
		debugger
	}
}

export default ChatWorker;