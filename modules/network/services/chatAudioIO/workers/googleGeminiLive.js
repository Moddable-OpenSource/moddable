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

class GoogleGeminiLiveModel extends ChatWebSocketWorker {
	constructor(options) {
		super(options);
		this.host = "generativelanguage.googleapis.com";
		this.path = `/ws/google.ai.generativelanguage.v1alpha.GenerativeService.BidiGenerateContent?key=${config.geminiAPIKey}`;
		this.headers = null;
		this.audioPrefix = ArrayBuffer.fromString('{"realtimeInput":{"mediaChunks":[{"mimeType":"audio/pcm;rate=24000","data":"');
		this.audioSuffix = ArrayBuffer.fromString('"}]}}');
		this.speaking = true;
	}
	configure(message) {
		const instructions = message.instructions ?? "";
		const tools = message.functions ?? [];
		this.setup = {
			model: "models/gemini-2.0-flash-exp",
			generationConfig: {
				responseModalities: "audio",
				speechConfig: {
					voiceConfig: { prebuiltVoiceConfig: { voiceName: "Aoede" } },
				},
			},
			systemInstruction: {
				parts: [
					{ text: instructions },
				],
			},
			tools: {
				functionDeclarations: tools
			}
		};
	}
	isBase64(result, current, name) {
		return (current?.mimeType == "audio/pcm;rate=24000") && (name == "data");
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
			const part = parts.find(part => part.inlineData?.mimeType == "audio/pcm;rate=24000");
			if (part) {
				if (this.speaking) {
					this.speaking = false;
					this.post("listen");
				}
			}
		}
		if (data.turnComplete) {
			this.parser.copy(this.silence);
			this.parser.done();
			this.speaking = true;
			this.post("speak");
		}
	}
	'setupComplete'(data) {
		this.post("connected");
	}
	'toolCall'(data) {
		const functionCalls = data.functionCalls;
		if (functionCalls) {
			this.post("listen");
			const functionResponses = [];
			for (let functionCall of functionCalls) {
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

new GoogleGeminiLiveModel({});
