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

const audioPrefix = Object.freeze(new Uint8Array(ArrayBuffer.fromString('{"type":"audio_input","data":"')), true);
const audioSuffix = Object.freeze(new Uint8Array(ArrayBuffer.fromString('"}')), true);

export default class HumeAIEVIModel extends ChatWebSocketWorker {
	constructor(options) {
		super(options);
		this.host = "api.hume.ai";
		this.headers = null;
		this.audioPrefix = audioPrefix;
		this.audioSuffix = audioSuffix;
		this.speaking = true;
	}
	connect(message) {
		const client = new device.network.https.io({ 
			...device.network.https,
			host: this.host
		});
		const headers = this.headers
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
					request("GET", "/v0/evi/configs?name=Moddable", null);
					return;
				case 1: 
					if (json?.fault) {
						this.postMessage({ id:"failed", string:json.fault.faultstring });
						return void client.close();
					}
					if (json && json.configs_page && (json.configs_page.length > 0)) {
						request("DELETE", `/v0/evi/configs/${json.configs_page[0].id}`, null);
						return;
					}
					break;
				case 2: 
					const body = ArrayBuffer.fromString(JSON.stringify(this.body));
					request("POST", "/v0/evi/configs", body);
					return;
				case 3: 
					client.close();
					this.path += `&config_id=${json.id}`;
					this.headers.delete("content-length");
					this.headers.delete("Content-Type");
					super.connect(message);
					return;
				}
			}
		};
		callback(null);
	}
	configure(message) {
		const instructions = message.instructions ?? "";
		const tools = message.functions ?? [];
		tools.forEach(tool => {
			tool.type = "function";
			tool.parameters = JSON.stringify(tool.parameters);
		});
 		this.session = {
			type: "session_settings",
			audio: {
				channels: 1,
				encoding: "linear16",
				sample_rate: 8000
			},
			system_prompt: instructions,
			tools,
		};
		const apiKey = message.apiKey ?? config.humeAIKey;
		this.path = `/v0/evi/chat?api_key=${apiKey}`;
		this.headers = new Map([
			[ "X-Hume-Api-Key", apiKey ],
			[ "Content-Type", "application/json" ],
		]);
 		this.body = {
			evi_version: "4-mini",
  			name: "Moddable",
			language_model: {
				model_provider: message.providerID ?? "ANTHROPIC",
				model_resource: message.modelID ?? "claude-haiku-4-5-20251001",
// 				temperature: 1
			},
  		};
		if (message.voiceID)
			this.body.voice = {
   				 provider: "HUME_AI",
   				 id: message.voiceID,
			}
	}
	isBase64(result, current, name) {
		return (result?.type == "audio_output") && (name == "data") ? 44 : false;
	}	
	sendAudio(message) {
		if (this.speaking)
			super.sendAudio(message);
	}
	sendFunctionResult(message) {
		const { call, result } = message;
		this.sendJSON({ type:'tool_response', tool_call_id: call, content: JSON.stringify(result) });
	}
	sendText(message) {
		this.sendJSON({ type:'user_input', text: message.text });
	}
	'assistant_end'(json) {
		this.parser.copy(this.silence);
		this.parser.done();
		this.speaking = true;
		this.post("speak");
	}
	'assistant_message'(json) {
		const message = json.message;
		if (message.role == "assistant") {
			this.postMessage({ id:"receiveOutputText", text:message.content });
		}
	}
	'audio_output'(json) {
		if (this.speaking) {
			this.speaking = false;
			this.post("listen");
		}
	}
	'chat_metadata'(json) {
 		this.sendJSON(this.session);
		this.post("connected");
	}
	'error'(json) {
		this.postMessage({ id:"failed", string:json.message });
		this.close();
	}
	'tool_call'(json) {
		if (this.speaking) {
			this.speaking = false;
			this.post("listen");
		}
		this.postMessage({ 
			id:"receiveFunctionCall", 
			call: json.tool_call_id,
			name:json.name, 
			parameters:JSON.parse(json.parameters)
		});
	}
	'user_message'(json) {
		if (json.interim)
			return;
		this.postMessage({ id:"receiveInputText", text:json.message.content });
	}
}

