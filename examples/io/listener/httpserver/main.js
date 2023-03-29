/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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

import HTTPServer from "embedded:network/http/server"
import Listener from "embedded:io/socket/listener";

import ServerSentEvents from "embedded:network/http/server/options/serversendevents";
import WebPage from "embedded:network/http/server/options/webpage";
import WebSocketHandshake from "embedded:network/http/server/options/websocket";

import WebSocket from "WebSocket";

const router = new Map;
const notFound = {
	...WebPage,
	msg: ArrayBuffer.fromString("Not found"),
};

let server = new HTTPServer({
	io: Listener,
	port: 80,
	onConnect(connection) {
		connection.accept({
			onRequest(request) {
				this.route = router.get(request.path) ?? notFound;
			},
		})
	}
})

const reply = ArrayBuffer.fromString("1 2 3 4 5 6 7 8\n");
router.set("/", {
	onRequest(request) {
		trace(`${request.method} ${request.path}\n`);
		for (const [header, value] of request.headers)
			trace(`${header}: ${value}\n`);				
	},
	onReadable(count) {
		trace(`${String.fromArrayBuffer(this.read(count))}\n`);
	},
	onResponse(response) {
		response.headers.set("content-length", reply.byteLength);
		this.respond(response);
	},
	onWritable(count) {
		this.write(reply);
	},
	onDone() {
		trace("done\n");
	},
	onError() {
		trace("error\n");
	}
});

/*
	Server-Sent Events server endpoint
*/
router.set("/sse", { 
	...ServerSentEvents,
	onConnection() {
		this.write({
			data: "one two three"
		});
		this.write({
			data: "one\n1\nwon",
			event: "event-one"
		});
	},
	onDone() {
		debugger;
	},
	onError() {
		debugger;
	}
});
router.set("/sse.html", { 
	...WebPage,
	msg: ArrayBuffer.fromString(`
<script>
console.log("starting EventSource");
var es = new EventSource("http://localhost/sse");
es.onmessage = function (event) {
  console.log(event.data);
};
es.addEventListener("event-one", (e) => {
  console.log("event-one: " + e.data)
})
es.addEventListener("event-two", (e) => {
  console.log("event-two: " + e.data)
})

</script>  
Hullo SSE
`)
});

/*
	WebSocket server endpoint
*/

router.set("/ws", { 
	...WebSocketHandshake, 
	onDone() {
		const ws = new WebSocket({attach: this.detach()});
		ws.addEventListener("open", () => {
			ws.send("HELLO");
		});
		ws.addEventListener("message", event => {
			let data = event.data;
			if (data instanceof ArrayBuffer)
				trace(`onmessage binary ${data.byteLength}\n`);
			else
				trace(`onmessage ${data}\n`);
			ws.send("GOODBYE");
		});
	},
});

/*
	WebSocket client for testing
*/
const ws = new WebSocket("ws://localhost/ws");
ws.binaryType = "arraybuffer";
ws.addEventListener("open", event => {
	ws.send("hello");
});
ws.addEventListener("message", event => {
	let data = event.data;
	if (data instanceof ArrayBuffer)
		trace(`CLIENT binary ${data.byteLength}\n`);
	else {
		trace(`CLIENT ${data}\n`);
// 		if (data.toUpperCase() == "GOODBYE")
// 			ws.close(1000, "Done");
	}
});
