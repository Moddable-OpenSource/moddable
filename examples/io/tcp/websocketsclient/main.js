/*
 * Copyright (c) 2021-2024  Moddable Tech, Inc.
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

const apiKey = "[[PUT YOUR OPENAI KEY HERE]]";

const WebSocketClient = device.network.wss.io;
const ws = new WebSocketClient({
	...device.network.wss,
	host: "api.openai.com",
	path: "/v1/realtime?model=gpt-4o-realtime-preview-2024-10-01",
	port: 443,
	headers: [
		["OpenAI-Beta", "realtime=v1"],
		["Authorization", `Bearer ${apiKey}`],
	],
	onReadable(count, options) {
		trace(String.fromArrayBuffer(this.read()));
		if (!options.more)
			trace("\n");
	},
	onControl(opcode, data) {
		switch (opcode) {
			case WebSocketClient.close: 
				trace("** Connection Closing **\n");
				break;

			case WebSocketClient.ping:
				trace("PING!\n");
				break;

			case WebSocketClient.pong:
				trace("PONG!\n");
				break;
		}
	},
	onClose() {
		trace("** Connection Closed **\n");
		
	},
	onError() {
		trace("** Connection Error **\n");
	}
});
