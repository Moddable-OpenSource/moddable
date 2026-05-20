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

import config from "mc/config"
import ChatWebSocketWorker from "ChatWebSocketWorker";

const audioPrefixOld = Object.freeze(new Uint8Array(ArrayBuffer.fromString('{"realtimeInput":{"mediaChunks":[{"mimeType":"audio/pcm;rate=24000","data":"')), true);
const audioSuffixOld = Object.freeze(new Uint8Array(ArrayBuffer.fromString('"}]}}')), true);
const audioPrefix = Object.freeze(new Uint8Array(ArrayBuffer.fromString('{"realtimeInput":{"audio":{"mimeType":"audio/pcm;rate=24000","data":"')), true);
const audioSuffix = Object.freeze(new Uint8Array(ArrayBuffer.fromString('"}}}')), true);

export default class GoogleGeminiLiveModel extends ChatWebSocketWorker {
	constructor(options) {
		super(options);
		this.host = "generativelanguage.googleapis.com";
		this.headers = null;
		this.speaking = true;
	}
	configure(message) {
		const instructions = message.instructions ?? "";
		const tools = message.functions ?? [];
		const voiceName = message.voiceID ?? "aoede";
		const model = message.modelID ?? "gemini-2.5-flash-native-audio-preview-12-2025";
		const apiKey = message.apiKey ?? config.geminiAPIKey;

		if(model.includes("gemini-2")) {
			this.audioPrefix = audioPrefixOld;
			this.audioSuffix = audioSuffixOld;
		} else {
			this.audioPrefix = audioPrefix;
			this.audioSuffix = audioSuffix;
		}

		this.path = `/ws/google.ai.generativelanguage.v1beta.GenerativeService.BidiGenerateContent?key=${apiKey}`;
		this.setup = {
			model: `models/${model}`,
			generationConfig: {
				responseModalities: "audio",
				speechConfig: {
					voiceConfig: { prebuiltVoiceConfig: { voiceName } },
				},
			},
			systemInstruction: {
				parts: [
					{ text: instructions },
				],
			},
			tools: {
				functionDeclarations: tools
			},
			inputAudioTranscription: {
			},
			outputAudioTranscription: {
			},
		};
	}
	isBase64(result, current, name) {
		return (current?.mimeType === "audio/pcm;rate=24000") && (name === "data");
	}
	onJSON(json) {
		for (let key in json) {
			if (key in this) {
				this[key](json[key]);
				break;
			}
		}
	}
	onOpen() {
		this.sendJSON({
			setup: this.setup
		});
	}
	sendAudio(message) {
		if (this.speaking)
			super.sendAudio(message);
	}
	sendFunctionResult(message) {
		const { call, result } = message;
		this.sendJSON({
			toolResponse: {
				functionResponses: [
					{
						id: call, 
						response: { output: result }
					}			
				]
			}
		});
	}
	sendText(message) {
		this.sendJSON({
			clientContent: {
				turns: [
					{
						role: "user",
						parts: [
							{ text: message.text },
						]
					}
				],
				turnComplete: true
			}
		});
	}
	'serverContent'(data) {
		const parts = data.modelTurn?.parts;
		if (parts) {
			const part = parts.find(part => part.inlineData?.mimeType === "audio/pcm;rate=24000");
			if (part) {
				if (this.speaking) {
					this.postMessage({ id:"receiveInputText", text:"" });
					this.speaking = false;
					this.post("listen");
				}
			}
		}
		if (data.turnComplete) {
			if (!this.speaking) {
				this.parser.copy(this.silence);
				this.parser.done();
				this.speaking = true;
				this.post("speak");
				this.postMessage({ id:"receiveOutputText", text:"" });
			}
		}
		if (data.inputTranscription) {
			this.postMessage({ id:"receiveInputText", text:data.inputTranscription.text, more:true });
		}
		if (data.outputTranscription) {
			this.postMessage({ id:"receiveOutputText", text:data.outputTranscription.text, more:true });
		}
	}
	'setupComplete'(/* data */) {
		this.post("connected");
	}
	'toolCall'(data) {
		const functionCalls = data.functionCalls;
		if (functionCalls) {
			this.post("listen");
			for (const functionCall of functionCalls) {
				this.postMessage({ 
					id:"receiveFunctionCall", 
					call:functionCall.id, 
					name: functionCall.name, 
					parameters: functionCall.args
				});
			}
		}
	}
}
