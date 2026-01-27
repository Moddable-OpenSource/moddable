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
 
import config from "mc/config"
import ChatWebSocketWorker from "ChatWebSocketWorker";
import Timer from "timer";
import {Encode} from "ChatAudioIO/Codecs";

class DeepgramVoiceAgentModel extends ChatWebSocketWorker {
	constructor(options) {
		super(options);
		this.host = "agent.deepgram.com";
		this.path = `/v1/agent/converse`;
		this.keepAliveTimer = null;
	}
	close() {
		this.stopKeepAlive();
		super.close();
	}
	configure(message) {
		const prompt = message.instructions ?? "";
		const functions = message.functions ?? [];
		const apiKey = message.apiKey ?? config.deepgramKey;
		this.headers = [
			["Authorization", `Token ${apiKey}`],
		];
		this.setup = {
			type: "Settings",
			experimental: true,
			audio: {
				input: {
					encoding: "alaw",
					sample_rate: 8000
				},
				output: { 
					encoding: "linear16",
					sample_rate: 16000,
				}
			},
			agent: {
				listen: {
					provider: {
						type: "deepgram",
						model: "flux-general-en",
					},
				},
				think: {
					provider: {
						type: message.providerID ?? "anthropic",
						model: message.modelID ?? "claude-3-5-haiku-latest",
					},
					prompt,
					functions,
				},
				speak: {
					provider: {
						type: "deepgram",
						model: message.voiceID ?? "aura-2-arcas-en",
					},
				}
			}
		}
	}
	isBase64(result, current, name) {
		return false;
	}
	listened(message) {
		this.stopKeepAlive();
	}
	read(data, options) {
		if (options.binary)
			this.parser.copy(data);
		else
			super.read(data, options);
	}
	sendAudio(message) {
// 		trace(`=> sendAudio ${ message.offset } ${ message.size }\n`);
		const data = new Uint8Array(this.inputBuffer, message.offset, message.size);
		Encode.toAlaw(data, data);
		this.write(data.subarray(0, message.size >> 1), { binary: true });
	}
	sendFunctionResult(message) {
		const { call, name, result } = message;
		this.sendJSON({ type:'FunctionCallResponse', id:call, name, content: result });
	}
	sendText(message) {
		this.sendJSON({ type:'InjectUserMessage', content:message.text  });
	}
	startKeepAlive() {
 		this.keepAliveTimer = Timer.repeat(timer => {
 			this.sendJSON({ type:'KeepAlive' });
 		}, 5000);
	}
	stopKeepAlive() {
		Timer.clear(this.keepAliveTimer);
		this.keepAliveTimer = null;
	}
	'AgentAudioDone'(message) {
		this.parser.copy(this.silence);
		this.parser.done();
		this.post("speak");
	}
	'AgentStartedSpeaking'(message) {
		this.post("listen");
		this.startKeepAlive();
	}
	'ConversationText'(message) {
		if (message.role == "user")
			this.postMessage({ id:"receiveInputText", text:message.content });
		else if (message.role == "assistant")
			this.postMessage({ id:"receiveOutputText", text:message.content });
	}
	'Error'(message) {
		this.postMessage({ id:"failed", string: message.description });
		this.close();
	}
	'FunctionCallRequest'(message) {
		message.functions.forEach(item => {
			this.postMessage({ 
				id:"receiveFunctionCall", 
				call: item.id,
				name: item.name, 
				parameters: JSON.parse(item.arguments)
			});
		});
	}
	'SettingsApplied'(message) {
		this.post("connected");
	}
	'Welcome'(message) {
 		this.sendJSON(this.setup);
	}
}

new DeepgramVoiceAgentModel({
	inputSampleRate: 8000,
	outputSampleRate: 16000
});
