/*
 * Copyright (c) 2020-2023  Moddable Tech, Inc.
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

import Resource from "Resource";
import {Server as HTTPServer} from "http/zipresource";
import {Server as WebSocketServer} from "websocket";

class WS extends WebSocketServer {
	#connections = [];
	
	constructor(options) {
		super(options);
		
		this.callback = this.cb;
	}
	cb(message, value) {
		const server = this.server;

		switch (message) {
			case WebSocketServer.connect:
				this.server = value;
				break;

			case WebSocketServer.handshake:
				server.#connections.push(this);
				trace(`WebSocket connected: ${server.#connections.length}\n`);
				break;

			case WebSocketServer.disconnect:
			case WebSocketServer.error:
				value = server.#connections.findIndex(value => value === this);
				if (-1 === value)
					return;

				server.#connections.splice(value, 1);
				trace(`WebSocket disconnected: ${server.#connections.length}\n`);
				break;

			case WebSocketServer.receive:
				try {
					trace(`WebSocket receive: ${value}\n`);
					value = JSON.parse(value);
					
					//@@ echo test
					value.echo = (new Date).toString();
					server.broadcast(value);
				}
				catch (e) {
					trace(`WebSocket parse received daa error: ${e}\n`);
				}
				break;
		}
	}
	broadcast(message) {
		message = JSON.stringify(message);
		trace(`WebSocket send: ${message}\n`);

		for (let i = 0, connections = this.#connections; i < connections.length; i++) {
			try {
				connections[i].write(message);
			}
			catch {
				connections.splice(i, 1);
				trace(`WebSocket send error: ${this.#connections.length}\n`);
			}
		}				
	}
}


const http = new HTTPServer({
	port: 80,
	onRoute(path) {
		if ("/" === path)
			path = '/index.html';
		return "/site" + path;
	}
}, new Resource("site.zip"));

const ws = new WS({
	port: 8080
});
