/*
 * Copyright (c) 2024-2026 Moddable Tech, Inc.
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

import AudioIn from "embedded:io/audio/in";
import AudioOut from "embedded:io/audio/out";
import Worker from "worker";

function computeLevel(buffer) { return native("xs_computeLevel").call(this, buffer); };

class ChatAudioIO {
	static FAILED = -1;
	static DISCONNECTED = 0;
	static DISCONNECTING = 1;
	static CONNECTING = 2;
	static CONNECTED = 3;
	static SPEAKING = 4;		// user is speaking (sending audio to cloud)
	static LISTENING = 5;		// user is listening (receiving audio from cloud) 
	static WAITING = 6;
	static states = [
		"DISCONNECTED",
		"DISCONNECTING",
		"CONNECTING",
		"CONNECTED",
		"SPEAKING",
		"LISTENING",
		"WAITING"
	];
	static {
		ChatAudioIO.states[-1] = "FAILED";
		Object.freeze(ChatAudioIO.states);
	}

	constructor(options) {
		this.error = "";
		this.state = ChatAudioIO.DISCONNECTED;

		this.input = null;
		this.inputBufferOffset = 0;
		this.inputBufferSize = 512 * 1024;
		this.inputBuffer = new SharedArrayBuffer(this.inputBufferSize);
		this.inputSampleRate = 24000;
		this.ready = false;
		
		this.output = null;
		this.outputBufferSize = 512 * 1024;
		this.outputBuffer = new SharedArrayBuffer(this.outputBufferSize);
		this.outputSampleRate = 24000;
   		this.outputBufferHead = 0;
		this.outputBufferTail = 0;
 		this.barrier = new Int32Array(new SharedArrayBuffer(4));
      
		this.level = 0;
		this.microphone = 1;
		this.volume = 1;
		
		const callback = () => {};
		this.onFunctionCall = options.onFunctionCall ?? callback;
		this.onInputLevelChanged = options.onInputLevelChanged ?? callback;
		this.onInputTranscript = options.onInputTranscript ?? callback;
		this.onOutputLevelChanged = options.onOutputLevelChanged;
		this.onOutputTranscript = options.onOutputTranscript ?? callback;
		this.onStateChanged = options.onStateChanged ?? callback;

		this.createWorker(options.specifier, options.instructions, options.functions, options.voiceID, options.providerID, options.modelID, options.apiKey);
	}
	close() {
		this.worker?.terminate();
		this.worker = null;
		this.output?.close();
		this.output = null;
		this.input?.close();
		this.input = null;
	}
	changeMicrophone(microphone) {
		this.microphone = microphone;
	}
	changeVolume(volume) {
		this.volume = volume;
		if (this.output)
			this.output.volume = volume;
	}
	createWorker(specifier, instructions, functions, voiceID, providerID, modelID, apiKey) {
		this.worker = new Worker(specifier, {
			static: 512 * 1024,
			chunk: {
				initial: 64 * 1024,
				incremental: 8 * 1024
			},
			heap: {
				initial: 1024,
				incremental: 256
			},
			stack: 1024,
			nativeStack: 12 * 1024
		});
		this.worker.onmessage = message => {
			this[message.id](message);
		};
		this.worker.postMessage({ 
			id: "configure", 
			instructions,
			functions,
			voiceID,
			providerID,
			modelID,
			apiKey
		})
 		this.ensureInput();
	}
	configureAudio(message) {
		const inputSampleRate = message.inputSampleRate ?? 24000;
		if (this.inputSampleRate != inputSampleRate) {
			this.inputSampleRate = inputSampleRate;
			if (this.input) {
				this.input.close();
				this.input = null;
				this.ensureInput();
			}
		}
		const outputSampleRate = message.outputSampleRate ?? 24000;
		if (this.outputSampleRate != outputSampleRate) {
			this.outputSampleRate = outputSampleRate;
			if (this.output) {
				this.output.close();
				this.output = null;
				this.ensureOutput();
			}
		}
	}
	connect() {
		this.state = ChatAudioIO.CONNECTING;
		this.inputBufferOffset = 0;
		Atomics.store(this.barrier, 0, 0);
		this.worker.postMessage({ id:"connect", inputBuffer: this.inputBuffer, outputBuffer: this.outputBuffer, barrier: this.barrier });
		this.onStateChanged(this.state);
	}
	connected() {
		this.state = ChatAudioIO.CONNECTED;
		this.onStateChanged(this.state);
		this.state = ChatAudioIO.SPEAKING;
		if (this.ready)
			this.onStateChanged(this.state);
	}
	disconnect() {
		this.state = ChatAudioIO.DISCONNECTING;
		this.worker.postMessage({ id:"disconnect" });
		this.onStateChanged(this.state);
	}
	disconnected() {
		this.error = "";
		this.state = ChatAudioIO.DISCONNECTED;
		this.ensureInput();
		this.onStateChanged(this.state);
	}
	failed(message) {
		this.error = message.string;
		this.state = ChatAudioIO.FAILED;
		this.ensureInput();
		this.onStateChanged(this.state);
	}
	listen() {
		if (this.state == ChatAudioIO.SPEAKING) {
			this.state = ChatAudioIO.LISTENING;
// 			this.outputBufferStart = undefined;
			this.ensureOutput();
			this.onStateChanged(this.state);
		}
	}
	receiveAudio(message) {
		this.outputBufferHead = message.offset + message.size;
	}
	receiveFunctionCall(message) {
		this.onFunctionCall(message.call, message.name, message.parameters);
	}
	receiveInputText(message) {
		this.onInputTranscript(message.text, message.more);
	}
	receiveOutputText(message) {
		this.onOutputTranscript(message.text, message.more);
	}
	sendFunctionResult(call, name, result) {
		this.worker.postMessage({ id: "sendFunctionResult", call, name, result });
	}
	sendText(text) {
		if (this.state < ChatAudioIO.CONNECTED)
			throw new Error("not connected");
		if (this.state > ChatAudioIO.SPEAKING)
			throw new Error("listening");
		this.worker.postMessage({  id: "sendText", text });
	}
	speak() {
		this.state = ChatAudioIO.WAITING;
	}
	
	ensureInput() {
		if (this.input) return;
		this.output?.close();
		this.output = null;
		let when = Date.now() + 500;
		this.input = new AudioIn({
			sampleRate: this.inputSampleRate,
			onReadable: (size) => {
				if (!this.ready) {
					if (Date.now() >= when) {
						this.ready = true;
						if (this.state != ChatAudioIO.DISCONNECTED)
							this.onStateChanged(this.state);
					}
					else
						return;
				}
				if (!this.microphone)
					return;
				let delta = this.inputBufferSize - this.inputBufferOffset;
				if (delta < size) {
					this.inputBufferOffset = 0;
					delta = this.inputBufferSize;
				}
				const samples = new Uint8Array(this.inputBuffer, this.inputBufferOffset, size);
				this.input.read(samples);
				const level = computeLevel(samples);
				if (this.level != level) {
					this.level = level;
					this.onInputLevelChanged(level);
				}
				if (this.state == ChatAudioIO.SPEAKING) {
					this.worker.postMessage({ id:"sendAudio", offset:this.inputBufferOffset, size });
				}
				this.inputBufferOffset += size;
			},
		});
		this.input.start(); 
	}
	ensureOutput() {
		if (this.output) return;
		this.input?.close();
		this.input = null;
		this.ready = false;
		this.output = new AudioOut({
			sampleRate: this.outputSampleRate,
			onWritable: (size) => {
				let start = this.outputBufferTail;
				let stop = this.outputBufferHead;
				let level = 0;
				if (stop < start) {
					let limit = this.outputBufferSize;
					if ((start < limit) && (size > 0)) {
						let delta = limit - start;
						if (delta > size)
							delta = size;
						const samples = new Uint8Array(this.outputBuffer, start, delta);
						if (this.onOutputLevelChanged) {
							const samplesLevel = computeLevel(samples);
							if (level < samplesLevel)
								level = samplesLevel;
						}
					this.output.write(samples);
						start += delta;
						size -= delta;
					}
					if (start == limit)
						start = 0;	
				}
				if ((start < stop) && (size > 0)) {
					let delta = stop - start;
					if (delta > size)
						delta = size;
					const samples = new Uint8Array(this.outputBuffer, start, delta);
					if (this.onOutputLevelChanged) {
						const samplesLevel = computeLevel(samples);
						if (level < samplesLevel)
							level = samplesLevel;
					}
					this.output.write(samples);
					start += delta;
					size -= delta;
				}
				this.outputBufferTail = start;
				Atomics.store(this.barrier, 0, start);
				Atomics.notify(this.barrier, 0);

				if ((start == stop) && (this.state == ChatAudioIO.WAITING)) {
					this.worker.postMessage({ id:"listened" });
					this.state = ChatAudioIO.SPEAKING;
					this.ensureInput();
				}
				if (this.level != level) {
					this.level = level;
					this.onOutputLevelChanged?.(level);
				}
			},
		});
		this.output.start();
		this.output.volume = this.volume;
	}
}

export default ChatAudioIO;
