/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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

import {Server as HTTPServer} from "http"
import {Server as WebsocketsServer} from "websocket"

const indexHTML = `
<html>
<head>
	<meta charset="utf-8">
	<title>test</title>
</head>
<body>

<script type="module" src="./ws.js"></script>
</body>
</html>
`;

const wsJS = `
const url = "ws://" + window.location.hostname + "/ws";
const ws = new WebSocket(url);

ws.onopen = function() {
	console.log("ws open");
	ws.send(JSON.stringify({"hello": "world"}));
}

ws.onclose = function() {
	console.log("ws close");
}

ws.onerror = function() {
	console.log("ws error");
}

ws.onmessage = function(event) {
	const data = event.data;
	console.log("ws message: ", data);
}
`;

// WebSockets server without a listener (signaled with port set to null)
// the http server hands-off incoming connections to this server
const websockets = new WebsocketsServer({port: null});
websockets.callback = function (message, value) {
	switch (message) {
		case WebsocketsServer.connect:
			trace("ws connect\n");
			break;

		case WebsocketsServer.handshake:
			trace("ws handshake\n");
			break;

		case WebsocketsServer.receive:
			trace(`ws message received: ${value}\n`);
			break;

		case WebsocketsServer.disconnect:
			trace("ws close\n");
			break;
	}
};

// HTTP server on port 80
const http = new HTTPServer;
http.callback = function(message, value, etc) {
	if (HTTPServer.status === message) {
		this.path = value;

		// request for "/ws" is handed off to the WebSockets server
		if ("/ws" === value) {
			const socket = http.detach(this);
			websockets.attach(socket);
		}

		return;
	}

	// return "/", "/index.html" and "/ws.js". all other paths are 404
	if (HTTPServer.prepareResponse === message) {
		if (("/" === this.path) || ("/index.html" === this.path)) 
			return {headers: ["Content-type", "text/html"], body: indexHTML};
		if ("/ws.js" === this.path)
			return {headers: ["Content-type", "text/javascript"], body: wsJS};

		return {status: 404};
	}
}
