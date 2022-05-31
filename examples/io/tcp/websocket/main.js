/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
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

import WebSocket from "WebSocket";
import data from "data";

const ws = new WebSocket("ws://websockets.chilkat.io/wsChilkatEcho.ashx");
ws.binaryType = "arraybuffer";
ws.addEventListener("open", event => {
	ws.send("Hello");
	ws.send(data);
	ws.send("Goodbye");
//	if closed here, messages are sent but the message events are not fired
// 	ws.close(1000, "Done");
	trace(`bufferedAmount ${ws.bufferedAmount}\n`);
});
ws.addEventListener("message", event => {
	let data = event.data;
	if (data instanceof ArrayBuffer)
		trace(`onmessage binary ${data.byteLength}\n`);
	else {
		trace(`onmessage ${data}\n`);
		if (data.toUpperCase() == "GOODBYE")
			ws.close(1000, "Done");
	}
});
ws.addEventListener("close", event => {
	trace(`onclose ${event.code} ${event.reason}\n`);
});
