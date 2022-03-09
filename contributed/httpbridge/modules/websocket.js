/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
 * Copyright (c) Wilberforce
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

import {Bridge, HTTPServer} from "bridge/webserver";
import {Server as WebSocketServer} from "websocket";

class WebSocketUpgrade extends WebSocketServer {
	static connections = [];
	#bridge;

	constructor(opts) {
		super(opts);
	}
	set bridge(value) {
		this.#bridge = value;
	}

	get bridge() {
		return this.#bridge;
	}

	callback(message, value) {
		const ws = this.ws;

		switch (message) {
			case WebSocketServer.connect:
				this.ws = value;
				break;

			case WebSocketServer.handshake:
				WebSocketUpgrade.connections.push(this);
				trace(`WebSocket connected: ${WebSocketUpgrade.connections.length}\n`);
				break;

			case WebSocketServer.disconnect:
			case WebSocketServer.error:
				value = WebSocketUpgrade.connections.findIndex(value => value === this);
				if (-1 === value)
					return;

				WebSocketUpgrade.connections.splice(value, 1);
				trace(`WebSocket disconnected: ${WebSocketUpgrade.connections.length}\n`);
				break;

			case WebSocketServer.receive:
				// App handles..
				break;
		}
		WebSocketUpgrade.bridge?.callback(ws,message, value ); // call app callback;
	}	

	broadcast(message, except) {
		message = JSON.stringify(message);
		trace(`WebSocket send: ${message}\n`);
		for (let i = 0, connections = WebSocketUpgrade.connections; i < connections.length; i++) {
			const connection = connections[i];

			if (except === connection) // Don't broadcast to self
				continue;
			try {
				//connection.send(message);
				connection.write(message);
			} catch {
				connections.splice(i, 1);
			}
		}
	}
}

class BridgeWebsocket extends Bridge {
	#path;
	#webserver;
	constructor( path='/') {
		super();
		this.#path=path;
	}

	get parent() {
		return this.#webserver;
	}
	set parent(p) {

		this.#webserver = p;
	}

	close() {
		for (let i = 0, connections = WebSocketUpgrade.connections; i < connections.length; i++) {
			try {
				connections[i].close()
			} catch {
				// Silently ignore
			}
		}
	}
	
	handler(req, message, value, etc) {
		switch (message) {
			case HTTPServer.status: 
				if ( value === this.#path ) {
					WebSocketUpgrade.bridge=this;
					const socket = this.parent.detach(req);
					const websocket = new WebSocketUpgrade({port:null});
					websocket.attach(socket);
					return;
				}
				break;
		}
		return this.next?.handler(req, message, value, etc);
	}
}

BridgeWebsocket.connect = 1;
BridgeWebsocket.handshake = 2;
BridgeWebsocket.receive = 3;
BridgeWebsocket.disconnect = 4;
BridgeWebsocket.subprotocol = 5;
BridgeWebsocket.encode = 6; // Websocket encode packet 
BridgeWebsocket.decode = 7; // Websocket decode packet 
Object.freeze(BridgeWebsocket.prototype);

export { BridgeWebsocket };
