/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

/*
	websocket client and server

	- validate Sec-WebSocket-Accept in client
	- messages with characters with ascii values above 255 will fail
*/

import {Socket, Listener} from "socket";
import Base64 from "base64";
import {Digest} from "crypt";

/*
	state:

		0 - connecting
		1 - sending handshake status
		2 - receving handshake headers
		3 - connected
		4 - done
		
	callback values:

		1 - connected socket
		2 - websocket handshake complete
		3 - message received
		4 - closed
*/

export class Client {
	constructor(dictionary) {
		// port, host, address, path (everything after port)
		this.path = dictionary.path ? dictionary.path : "/";
		this.host = dictionary.host ? dictionary.host : dictionary.address;
		this.headers = dictionary.headers ? dictionary.headers : [];
		this.protocol = dictionary.protocol;
		this.state = 0;

		if (dictionary.socket)
			this.socket = dictionary.socket;
		else {
			if (!dictionary.port) dictionary.port = 80;
			if (dictionary.Socket)
				this.socket = new dictionary.Socket(Object.assign({}, dictionary.Socket, dictionary));
			else
				this.socket = new Socket(dictionary);
		}
		this.socket.callback = callback.bind(this);
		this.doMask = true;
	}

	write(message) {
	//@@ implement masking
		let string = !(message instanceof ArrayBuffer);
		if (string)
			message = message.toString();

		let length = string ? message.length : message.byteLength;
		// Note: WS spec requires XOR masking for clients, but w/ strongly random mask. We
		// can't achieve that on this device for now, so just punt and use 0x00000000 for
		// a no-op mask.
		if (length < 126) {
			if (this.doMask)
				this.socket.write(string ? 0x81 : 0x82, length | 0x80, 0, 0, 0, 0, message);
			else
				this.socket.write(string ? 0x81 : 0x82, length, message);
		}
		else if (length < 65536) {
			if (this.doMask)
				this.socket.write(string ? 0x81 : 0x82, 126 | 0x80, length >> 8, length & 0x0ff, 0, 0, 0, 0, message);
			else
				this.socket.write(string ? 0x81 : 0x82, 126, length >> 8, length & 0x0ff, message);
		}
		else
			throw new Error("message too long");
	}

	close() {
		this.socket.close();
		delete this.socket;
	}

	mask() @ "xs_websocket_mask";
};

function callback(message, value) {
	let socket = this.socket;

	if (1 == message) {	// connected
		if (0 != this.state)
			throw new Error("socket connected but ws not in connecting state");

		this.callback(1);		// connected socket

		let key = new Uint8Array(16);
		for (let i = 0; i < 16; i++)
			key[i] = (Math.random() * 256) | 0

		let response = [
			"GET ", this.path, " HTTP/1.1\r\n",
			"Host: ", this.host, "\r\n",
			"Upgrade: websocket\r\n",
			"Connection: keep-alive, Upgrade\r\n",
			"Sec-WebSocket-Version: 13\r\n",
			"Sec-WebSocket-Key: ", Base64.encode(key.buffer) + "\r\n",
		];

		if (this.protocol)
			response.push(`Sec-WebSocket-Protocol: ${this.protocol}\r\n`);

		let hdr = undefined;
		if (this.headers) for (let w of this.headers) {
			if (hdr === undefined) {
				hdr = w;
			} else {
				response.push(`${hdr}: ${w}\r\n`);
				hdr = undefined;
			}
		}
		if (hdr != undefined)
			throw new Error("invalid header array: need a value for every header");

		response.push("\r\n");
		socket.write.apply(socket, response);

		delete this.path;
		delete this.host;
		delete this.headers;
		delete this.protocol;

		this.state = 1;
	}

	if (2 == message) {		// data available to read
		if (1 == this.state) {
			let line = socket.read(String, "\n");
			if ("HTTP/1.1 101" !== line.substring(0,12))
				throw new Error("web socket upgrade failed");
			this.state = 2;
			this.line = undefined;
			this.flags = 0;
		}
		if (2 == this.state) {
			while (true) {
				let line = socket.read(String, "\n");
				if (!line)
					return;			// out of data. wait for more.

				if (this.line) {
					line = this.line + line;
					this.line = undefined;
				}

				if (10 != line.charCodeAt(line.length - 1)) {		// partial header line, accumulate and wait for more
trace("partial header!!\n");		//@@ untested
					this.line = line;
					return;
				}

				if ("\r\n" == line) {		// empty line is end of headers
					if (7 == this.flags) {
						this.callback(2);		// websocket handshake complete
						this.state = 3;			// ready to receive
						value = socket.read();
					}
					else {
						this.callback(4);		// failed
						this.state = 4;			// close state
						return;
					}
					delete this.flags;
					delete this.line;
					value = socket.read();	// number of bytes available
					if (!value) return;
					break;
				}

				let position = line.indexOf(":");
				let name = line.substring(0, position).trim().toLowerCase();
				let data = line.substring(position + 1).trim();

				if ("connection" == name) {
					if ("upgrade" == data.toLowerCase())
						this.flags |= 1;
				}
				else if ("sec-websocket-accept" == name) {
					this.flags |= 2;		//@@ validate data
				}
				else if ("upgrade" == name) {
					if ("websocket" == data.toLowerCase())
						this.flags |= 4;
				}
			}
		}
		if (3 == this.state) {		// receive message
			let tag = socket.read(Number);
			let length = socket.read(Number);
			let mask = 0 != (length & 0x80);
			length &= 0x7f;
			if (126 == length) {
				length = socket.read(Number) << 8;
				length |= socket.read(Number);
			}
			else if (127 == length)
				; //@@ crazy unsupported 8 byte length

			switch (tag & 0x0f) {
				case 1:
				case 2:
					let data;
					if (mask) {
						mask = socket.read(ArrayBuffer, 4);
						data = this.mask(socket.read(ArrayBuffer, length), mask, (1 === (tag & 0x0f)) ? String : ArrayBuffer);
					}
					else
						data = socket.read((1 === (tag & 0x0f)) ? String : ArrayBuffer, length);
					this.callback(3, data);
					break;
				case 8:
					this.state = 4;
					this.callback(4);		// close
					this.close();
					return;
				case 9:		// ping
					if (length)
						socket.write(0x8a, length, socket.read(ArrayBuffer, length));		//@@ assumes length is 125 or less
					else
						socket.write(0x8a, 0);
					break;
				case 10:		// pong
					socket.read(null, length);
					break;
				default:
					trace("unrecognized frame type\n");
					break;
			}
		}
	}

	if (-1 == message) {
		if (4 !== this.state) {
			this.callback(4);
			this.close();
			this.state = 4;
		}
	}
}

export class Server {
	constructor(dictionary) {
		this.listener = new Listener({port: dictionary.port ? dictionary.port : 80});
		this.listener.callback = listener => {
			let socket = new Socket({listener: this.listener});
			let request = new Client({socket});
			request.doMask = false;
			socket.callback = server.bind(request);
			request.state = 1;		// already connected socket
			request.callback = this.callback;		// transfer server.callback to request.callback
			request.callback(1);		// tell app we have a new connection
		};
	}

	close() {
		this.listener.close();
		delete this.listener;
	}
};

/*
	callback for server handshake. after that, switches to client callback
*/

function server(message, value, etc) {
	let socket = this.socket;

	if (!socket) return;

	trace(`WS SERVER SOCKET MSG ${message} in state ${this.state}\n`);

	if (2 == message) {
		if ((1 == this.state) || (2 == this.state)) {
			while (true) {
				let line = socket.read(String, "\n");
				if (!line)
					return;			// out of data. wait for more.

				if (this.line) {
					line = this.line + line;
					this.line = undefined;
				}

				if (10 != line.charCodeAt(line.length - 1)) {		// partial header line, accumulate and wait for more
trace("partial header!!\n");		//@@ untested
					this.line = line;
					return;
				}

				if ("\r\n" == line) {		// empty line is end of headers
					if (15 !== this.flags)
						throw new Error("not a valid websocket handshake");

					delete this.line;
					delete this.flags;

					let sha1 = new Digest("SHA1");
					sha1.write(this.key);
					delete this.key;
					sha1.write("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

					socket.write("HTTP/1.1 101 Web Socket Protocol Handshake\r\n",
								"Connection: Upgrade\r\n",
								"Upgrade: websocket\r\n",
								"Sec-WebSocket-Accept: ", Base64.encode(sha1.close()), "\r\n",
								"\r\n");

					this.callback(2);		// websocket handshake complete

					this.state = 3;
					socket.callback = callback.bind(this);
					value = socket.read();	// number of bytes available
					if (0 !== value)		// should be 0. unexpected to receive a websocket message before server receives handshake
						socket.callback(2, value);
					return;
				}

				if (1 == this.state) {
					// parse status line: GET / HTTP/1.1
					line = line.split(" ");
					if (line.length < 3)
						throw new Error("unexpected status format");
					if ("GET" != line[0])
						throw new Error("unexpected GET");
					if ("HTTP/1.1" != line[line.length - 1].trim())
						throw new Error("HTTP/1.1");
					//@@ could provide path to callback here
					this.state = 2;
					this.flags = 0;
				}
				else if (2 == this.state) {
					let position = line.indexOf(":");
					let name = line.substring(0, position).trim().toLowerCase();
					let data = line.substring(position + 1).trim();

					if ("upgrade" === name)
						this.flags |= (data.toLowerCase() === "websocket") ? 1 : 0;
					else if ("connection" === name) {		// Firefox: "Connection: keep-alive, Upgrade"
						data = data.split(",");
						for (let i = 0; i < data.length; i++)
							this.flags |= (data[i].trim().toLowerCase() === "upgrade") ? 2 : 0;
					}
					else if ("sec-websocket-version" === name)
						this.flags |= (data.toLowerCase() === "13") ? 4 : 0;
					else if ("sec-websocket-key" === name) {
						this.flags |= 8;
						this.key = data;
					}
				}
			}
		}
	}

	if (-1 == message) {
		this.callback(4);
		this.close();
	}
}

Object.freeze(Server.prototype);
Object.freeze(Client.prototype);

export default {
	Client, Server
};
