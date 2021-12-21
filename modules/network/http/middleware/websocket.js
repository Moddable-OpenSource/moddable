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

import { Middleware, Server as HTTPServer } from "middleware/server";

// https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers
// https://github.com/Moddable-OpenSource/moddable/blob/public/modules/network/websocket/websocket.js#L375-L398

import Base64 from "base64";
import {Digest} from "crypt";
import {Client as WebSocketClient} from "websocket/websocket"

class WebSocketUpgrade extends WebSocketClient {
	constructor(opts) {
		super(opts);
		
		this.doMask = false;
		this.state = WebSocketClient.receive;
		opts.socket=null; // Nuke so http server does not close
	}

	send(packet) {
		packet=JSON.stringify(packet);
		let encoded = this.callback.call( this, MiddlewareWebsocket.encode, packet);
		if (encoded)
			packet = encoded;
		this.write(packet);
	}
}

class MiddlewareWebsocket extends Middleware {
	#path;
	#webserver;
	constructor( path='/') {
		super();
		this.#path=path
	}

	get parent() {
		return this.#webserver;
	}
	set parent(p) {

		this.#webserver = p;
	}

	callbackHandler( message, value ) {
		if ( message === WebSocketClient.receive ) {
			let decoded = this.callback.call( this, MiddlewareWebsocket.decode, value);
			if (decoded) {
				value = decoded;
			}
		}
		return this.callback.call( this, message, value ); // call app callback;
	}

	broadcast(message, except) {
		for (let i = 0, connections = this.#webserver.connections; i < connections.length; i++) {
			const connection = connections[i];

			if (except === connection) // Don't broadcast to self
				continue;
			if (connection instanceof WebSocketUpgrade) {
				try {
					connection.send(message);
				} catch {
					connections.splice(i, 1);
				}
			}
		}
	}

	close() {
		this.#webserver.close();
	}
	
	handler(req, message, value, etc) {
		switch (message) {
			case HTTPServer.status: 
				if ( value === this.#path ) {
					req.ws = { flags: 0 };
				}
			break;
			case HTTPServer.header: {
				if ( req.ws ) {
					if ( value==='connection' && etc === 'Upgrade')  {
						req.ws.flags |= 1
					}
					if ( value==='sec-websocket-version') {
						req.ws.flags |= (etc === '13') ? 2 : 0;
					}
					if ( ( value==='upgrade' && etc === 'websocket' ) ) { 
						req.ws.flags |= 4
					}					
					if ( value==='sec-websocket-key') {
						req.ws.flags |= 8;
						req.ws.key = etc;
						this.callback( WebSocketClient.connect ); // tell app we have a new connection
					}
					if ( value==='sec-websocket-protocol') {
						let data = etc.split(",");
						for (let i = 0; i < data.length; ++i)
							data[i] = data[i].trim().toLowerCase();
						let protocol = this.callback(MiddlewareWebsocket.subprotocol, data);
						if (protocol)
							req.ws.protocol = protocol;
					}
				}
				break; // continue chaining headers
			}
			case HTTPServer.prepareResponse:
				
				if ( req.ws && req.ws.flags === 15 ) { 
					let sha1 = new Digest("SHA1");
					
					sha1.write(req.ws.key);
					delete req.ws.key;
					delete req.ws.flags;
					sha1.write("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
					let accept = Base64.encode(sha1.close());
					let response = {
						headers: [ 
							"Sec-WebSocket-Accept", accept,
							"Upgrade","websocket"
						],
						status: 101
					};
					if (req.ws.protocol) {
						response.headers.push("Sec-WebSocket-Protocol",req.ws.protocol);
					}
					return response;
				}
				// ws handshake failure - continue chain
				delete req.ws;
				break;
			case HTTPServer.responseComplete:
				if ( req.ws ) {
					
					const websocket = new WebSocketUpgrade({socket:req.socket});
					websocket.callback=this.callbackHandler.bind(this);
					
					req.server.connections.push(websocket);
					this.callback.call( websocket, WebSocketClient.handshake ); // tell app we have handshake complete
					return;
				}
				break;
		}
		return this.next?.handler(req, message, value, etc);
	}
}

MiddlewareWebsocket.connect = 1;
MiddlewareWebsocket.handshake = 2;
MiddlewareWebsocket.receive = 3;
MiddlewareWebsocket.disconnect = 4;
MiddlewareWebsocket.subprotocol = 5;
MiddlewareWebsocket.encode = 6; // Websocket encode packet 
MiddlewareWebsocket.decode = 7; // Websocket decode packet 
Object.freeze(MiddlewareWebsocket.prototype);

export { MiddlewareWebsocket };

/*  TO DO

flags -> use constants ? also http module

method === 'GET'

*/
