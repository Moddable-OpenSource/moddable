/*
 * Copyright (c) 2025 Moddable Tech, Inc.
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

const audioPrefix = Object.freeze(new Uint8Array(ArrayBuffer.fromString(`{"user_audio_chunk":"`)), true);
const audioSuffix = Object.freeze(new Uint8Array(ArrayBuffer.fromString('"}')), true);

class ElevenLabsModel extends ChatWebSocketWorker {
	constructor(options) {
		super(options);
		this.host = "api.elevenlabs.io";
		this.path = "";
		this.headers = [
		];
        this.audioPrefix = audioPrefix;
		this.audioSuffix = audioSuffix;
		this.speaking = true;
	}
	configure(message) {
		const prompt = message.instructions ?? "";
		const tools = message.functions ?? [];
		tools.forEach(tool => {
			tool.type = "client";
			tool.id = this.generateId("tool");
		});
		this.setup = {
			type: "conversation_initiation_client_data",
		}
		this.body = {
			conversation_config: {
				asr: {
					user_input_audio_format:"pcm_16000",
				},
				tts: {
					agent_output_audio_format:"pcm_16000",
				},
				conversation: {
					client_events: [
						"audio",
						"interruption",
						"user_transcript",
						"agent_response",
						"client_tool_call",
					]
				},
				agent: {
					prompt: {
						prompt,
						llm: message.modelID ?? "claude-haiku-4-5",
						tools
					},
				},
			},
			name: "Moddable",
		};
		if (message.voiceID)
			this.body.conversation_config.tts.voice_id = message.voiceID;
	}
	connect(message) {
		const client = new device.network.https.io({ 
			...device.network.https,
			host: this.host
		});
		const headers = new Map([
			[ "xi-api-key", config.elevenLabsKey ],
			[ "Content-Type", "application/json" ],
		]);
		const request = (method, path, body) => {
			let buffer = null;
			let length = 0;
			let offset = 0;
			if (body) {
				length = body.byteLength;
				headers.set("content-length", length);
			}
			client.request({
				method, path, headers,
				onWritable(count) {
					if (body) {
						let remain = length - offset;
						if (remain > 0) {
							if (count > remain)
								count = remain;
							let view = new DataView(body, offset, count);
							this.write(view);
							offset += count;
						}
						else
							this.write();
					}
				},
				onReadable(count) {
					if (buffer)
						buffer = buffer.concat(this.read(count));
					else
						buffer = this.read(count);
				},
				onDone(error) {
					if (buffer)
						buffer = JSON.parse(String.fromArrayBuffer(buffer));
					callback(buffer);
				}
			})
		};
		let state = -1;
		const callback = json => {
			for (;;) {
				state++;
				switch (state) {
				case 0: 
					request("GET", "/v1/convai/agents", null);
					return;
				case 1: 
					if (json?.agents) {
						let agent = json.agents.find(agent => agent.name == "Moddable");
						if (agent) {
							request("DELETE", `/v1/convai/agents/${agent.agent_id}`, null);
							return;
						}
					}
					else {
						this.postMessage({ id:"failed", string:json?.detail?.message ?? "failed" });
						client.close();
						return;
					}
					break;
				case 2: 
					const body = ArrayBuffer.fromString(JSON.stringify(this.body));
					request("POST", "/v1/convai/agents/create", body);
					return;
				case 3: 
					client.close();
					if (json?.agent_id) {
						this.path = `/v1/convai/conversation?agent_id=${json.agent_id}`;
						super.connect(message);
					}
					else {
						this.postMessage({ id:"failed", string:json?.detail?.message ?? "failed" });
						client.close();
					}
					return;
				}
			}
		};
		callback(null);
	}
	generateId(prefix, length = 21) {
		// base58; non-repeating chars
		const chars = '123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz';
		const str = Array(length - prefix.length) 
		.fill(0)
		.map((_) => chars[Math.floor(Math.random() * chars.length)])
		.join('');
		return `${prefix}${str}`;
	}
	isBase64(result, current, name) {
		return (name == "audio_base_64");
	}	
	onBase64(offset, size) {
		super.onBase64(offset, size);
		if (this.speaking) {
			this.speaking = false;
			this.post("listen");
		}
	}
	sendFunctionResult(message) {
		const { call, result } = message;
		this.sendJSON({
			type: 'client_tool_result',
			tool_call_id: call,
			result: JSON.stringify(result),
  			is_error: false,
		});
	}
	sendText(message) {
		this.sendJSON({
			type: 'user_message',
			text: message.text,
		});
	}
	'agent_response'(message) {
		this.postMessage({ id:"receiveOutputText", text:message.agent_response_event.agent_response });
	}
	'audio'(message) {
	}
	'client_tool_call'(message) {
		const { client_tool_call } = message;
      	if (client_tool_call) {
			this.postMessage({ 
				id:"receiveFunctionCall", 
				call: client_tool_call.tool_call_id,
				name:client_tool_call.tool_name, 
				parameters:client_tool_call.parameters
			});
		}
	}
	'conversation_initiation_metadata'(message) {
		this.post("connected");
	}
	'ping'(message) {
		const event_id = message.ping_event.event_id;
		this.sendJSON({ type:"pong", event_id });
		if (this.speaking)
			return;
		this.parser.copy(this.silence);
		this.parser.done();
		this.speaking = true;
		this.post("speak");
	}
	'user_transcript'(message) {
		this.postMessage({ id:"receiveInputText", text:message.user_transcription_event.user_transcript });
	}
}

new ElevenLabsModel({
	inputSampleRate: 16000,
	outputSampleRate: 16000,
});
