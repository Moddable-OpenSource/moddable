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

const audioPrefix = Object.freeze(new Uint8Array(ArrayBuffer.fromString('{"type":"audio_input","data":"')), true);
const audioSuffix = Object.freeze(new Uint8Array(ArrayBuffer.fromString('"}')), true);

class HumeAIEVIModel extends ChatWebSocketWorker {
	constructor(options) {
		super(options);
		this.host = "api.hume.ai";
		this.path = `/v0/evi/chat?api_key=${config.humeAIKey}`;
		this.headers = null;
		this.audioPrefix = audioPrefix;
		this.audioSuffix = audioSuffix;
		this.speaking = true;
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

new HumeAIEVIModel({
	inputSampleRate: 8000
});
