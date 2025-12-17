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
import {Encode} from "ChatAudioIO/Codecs";

const audioPrefix = Object.freeze(new Uint8Array(ArrayBuffer.fromString(`{"type":"input_audio_buffer.append","audio":"`)), true);
const audioSuffix = Object.freeze(new Uint8Array(ArrayBuffer.fromString('"}')), true);

class OpenAIRealTimeModel extends ChatWebSocketWorker {
	constructor(options) {
		super(options);
		this.host = "api.openai.com";
		this.headers = [
			["Authorization", `Bearer ${config.openAIKey}`]
		];
        this.audioPrefix = audioPrefix;
		this.audioSuffix = audioSuffix;
	}
	configure(message) {
		const instructions = message.instructions ?? "";
		const tools = message.functions ?? [];
		const voice = message.voiceID ?? "marin";
		const model = message.modelID ?? "gpt-realtime-mini";
		this.path = `/v1/realtime?model=${model}`;
		tools.forEach(tool => {
			tool.type = "function";
			tool.parameters.additionalProperties = false;
		});
 		this.session = {
			type: 'realtime',
			audio: {
				input: {
					format: {
						type: "audio/pcma",
					},
					transcription: { model: 'whisper-1' },
					turn_detection: {
						type: "server_vad",
						threshold: 0.5,
						prefix_padding_ms: 300,
						silence_duration_ms: 500,
						create_response: true
					},
				},
				output: {
					format: {
						type: "audio/pcm",
						rate: 24000,
					},
					voice,
				}
			},
			instructions,
			tools,
			tool_choice: "auto",
		}
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
		return (result?.type == "response.output_audio.delta") && (name == "delta");
	}	
	sendAudio(message) {
		const buffer = new Uint8Array(this.inputBuffer, message.offset, message.size);
		Encode.toAlaw(buffer, buffer);
		message.size >>= 1; 
		return super.sendAudio(message);
	}
	sendFunctionResult(message) {
		const { call, result } = message;
		this.sendJSON({
			type: 'conversation.item.create',
			item: {
				type: 'function_call_output',
				call_id: call,
				output: JSON.stringify(result),
			},
			event_id: this.generateId('event_'),
		});
	}
	sendText(message) {
		this.sendJSON({
			type: 'conversation.item.create',
			item: {
				type: 'message',
				role: 'user',
				content: [
					{
						type: "input_text",
						text: message.text
					}
				]
			},
			event_id: this.generateId('event_'),
		});
	}
	onJSON(json) {
		if ("failed" === json.response?.status) {
			this.postMessage({ id:"failed", string: json.response.status_details?.error?.message ?? `${json.type} failed`});
			return void this.close();
		}
		return super.onJSON(json)
	}
	'conversation.item.input_audio_transcription.completed'(message) {
		this.postMessage({ id:"receiveInputText", text:message.transcript });
	}
	'error'(message) {
		this.postMessage({ id:"failed", string: message.error.message });
		this.close();
	}
	'input_audio_buffer.committed'(message) {
		this.post("listen");
	}
	'response.output_audio_transcript.delta'(message) {
		this.postMessage({ id:"receiveOutputText", text:message.delta, more:true });
	}
	'response.output_audio_transcript.done'(message) {
		this.postMessage({ id:"receiveOutputText", text:"" });
	}
	'response.created'(message) {
		this.postMessage({ id:"receiveInputText", text:"", more:true });
		this.postMessage({ id:"receiveOutputText", text:"", more:true });
	}
	'response.done'(message) {
		this.parser.copy(this.silence);
		this.parser.done();
		this.post("speak");
	}
	'response.output_item.done'(message) {
		const { item } = message;
      	if (item.type === 'function_call') {
			this.postMessage({ 
				id:"receiveFunctionCall", 
				call: item.call_id,
				name:item.name, 
				parameters:JSON.parse(item.arguments)
			});
		}
	}
	'session.created'(message) {
 		this.sendJSON({
 			type:'session.update', 
 			session:this.session,
 			event_id: this.generateId('event_'),
		});
	}
	'session.updated'(message) {
		this.post("connected");
	}
}

new OpenAIRealTimeModel({
	inputSampleRate: 8000
});
