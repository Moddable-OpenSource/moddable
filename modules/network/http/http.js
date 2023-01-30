/*
 * Copyright (c) 2016-2023 Moddable Tech, Inc.
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
	http
*/

import {Socket, Listener} from "socket";
import Timer from "timer";

/*
	Client

	state:
	
		0 - connecting
		1 - sending request headers
		2 - sending request body
		3 - receiving status
		4 - receiving response headers
		5 - receiving response body
		6 - done
		
	callback values:
	
		0 - get request body fragment
		1 - response status received with status code
		2 - one header received (name, value)
		3 - all headers received
		4 - part of response body
		5 - all body received
		6 - done
*/

export class Request {
	constructor(dictionary) {
		if (dictionary.socket) {
			this.socket = dictionary.socket;		// server re-using Request
			return;
		}

		dictionary = {port: 80, method: "GET", path: "/", Socket, ...dictionary};

		this.method = dictionary.method;
		this.path = dictionary.path;
		this.host = dictionary.host ? dictionary.host : dictionary.address;
		if (dictionary.headers)
			this.headers = dictionary.headers;
		if (dictionary.body)
			this.body = dictionary.body;
		if (dictionary.response)
			this.response = dictionary.response;
		this.state = 0;

		this.socket = new (dictionary.Socket)(dictionary);
		this.socket.callback = callback.bind(this);
	}

	read(type, limit) {
		let available = this.socket.read();

		if (undefined !== this.chunk) {
			if (undefined === limit)
				limit = this.chunk;
			else if (typeof limit !== "number")
				throw new Error("http limit only supports number");

			if (undefined === type)
				return limit;

			if (type === Number)
				limit = 1;
			else if (limit > this.chunk)
				limit = this.chunk;

			if (available < limit)
				limit = available;

			if (!limit)
				return;

			this.chunk -= limit;
		}
		else if (undefined === type)
			return available;

		const result = this.socket.read(type, limit);
		if (this.total) {
			this.total -= (available - this.socket.read());
			if (!this.total) {
				this.done = Timer.set(() => {
					if (this.done)
						done.call(this);
				});
			}
		}
		return result;
	}

	close() {
		this.socket?.close();
		delete this.socket;
		delete this.buffers;
		delete this.callback;
		delete this.done;
		delete this.server;
		this.state = 11;
	}
};
Request.requestFragment = 0;
Request.status = 1;
Request.header = 2;
Request.headersComplete = 3;
Request.responseFragment = 4;
Request.responseComplete = 5;
Request.error = -2;

function callback(message, value) {
	let socket = this.socket;

	if (1 === message) {	// connected
		if (0 !== this.state)
			throw new Error("socket connected but http not in connecting state");

		this.state = 1;		// advance to sending request headers state

		const parts = [];
		parts.push(`${this.method} `, this.path, " HTTP/1.1\r\n");
		parts.push("Connection: close\r\n");

		let length, host = this.host;
		for (let i = 0; i < (this.headers?.length ?? 0); i += 2) {
			let name = this.headers[i].toString();
			parts.push(`${name}: `, this.headers[i + 1].toString(), "\r\n");
			name = name.toLowerCase();
			if ("content-length" === name)
				length = true;
			else if ("host" === name)
				host = undefined;
		}

		if (this.body && (true !== this.body) && !length) {
			length = (this.body instanceof ArrayBuffer) ? this.body.byteLength : this.body.length;
			parts.push(`content-length: ${length}\r\n`);
		}

		if (host)
			parts.push(`Host: ${host}\r\n`);

		parts.push("\r\n");

		delete this.method;
		delete this.path;
		delete this.host;
		delete this.headers;

		this.parts = parts;

		message = 3;
		value = socket.write();
	}

	if (3 === message) {	// safe to write more data
		if (1 == this.state) {
			const p = [], parts = this.parts;
			while (value && parts.length) {
				const length = parts[0].length;
				if (length <= value) {
					p.push(parts.shift());
					value -= length;
				}
				else {
					p.push(parts[0].slice(0, value));
					parts[0] = parts[0].slice(value);
					value = 0;
				}
			}
			socket.write.apply(socket, p);
			if (parts.length) return;

			delete this.parts;

			this.state = (undefined === this.body) ? 3 : 2;		// if no body, advance to receiving status state, otherwise begin to transmit request body immediately, if possbile
		}

		if (2 === this.state) {
			if (true === this.body) {
				let body = this.callback(Request.requestFragment, socket.write());
				if (undefined !== body) {
					socket.write(body);
					return;
				}
			}
			else
				socket.write(this.body);

			delete this.body;
			this.state = 3;		// advance to receiving status state
			return;
		}
	}

	if (2 === message) {			// data available to read
		if (3 === this.state) {				// receiving status
			let status = socket.read(String, "\n");		// read up to the end of the line
			if (this.line)
				status = this.line + status;

			if ((10 !== status.charCodeAt(status.length - 1)) || (13 !== status.charCodeAt(status.length - 2))) {
				this.line = status;
				return;
			}

			// parse status line:  HTTP/1.1 200 OK
			status = status.split(" ");
			if (status.length < 3)
				throw new Error("unexpected status format");
			this.state = 4;			// advance to receiving response headers state
			delete this.line;		// no header line fragment yet

			this.total = undefined;		// total number of bytes expected (content-length)
			this.chunk = undefined;		// bytes remaining in this chunk (undefined if not chunked)

			this.callback(Request.status, parseInt(status[1]));

			if ((this.state >= 11) || !socket.read())
				return;
		}

		if (4 === this.state) {		// receiving response headers
			while (true) {
				let line = socket.read(String, "\n");
				if (!line)
					return;			// out of data. wait for more.

				if (this.line) {
					line = this.line + line;
					delete this.line;
				}

				if (10 !== line.charCodeAt(line.length - 1)) {		// partial header line, accumulate and wait for more
					this.line = line;
					return;
				}

				if ("\r\n" === line) {							// empty line is end of headers
					this.callback(Request.headersComplete);		// all response headers received
					delete this.line;
					if (this.state >= 11)
						return;		// callback closed instance
					this.state = 5;								// advance to receiving response headers state

					if (this.response)
						this.buffers = [];	// array to hold response fragments

					value = socket.read();	// number of bytes available
					if (0 === value) {
						if (0 === this.total) {
							delete this.total;
							done.call(this);
						}
						return;
					}
					break;
				}

				let position = line.indexOf(":");
				let name = line.substring(0, position).trim().toLowerCase();
				let data = line.substring(position + 1).trim();
				this.callback(Request.header, name, data);		// one received header

				if ("content-length" === name)
					this.total = parseInt(data);
				else if ("transfer-encoding" === name) {
					data = data.toLowerCase();
					if ("chunked" === data)
						this.chunk = 0;
					else if ("identity" === data)
						;
					else
						return done.call(this);
				}

				if (this.state >= 11)
					return;
			}
		}

		if (5 === this.state) {		// receiving response body
			if (undefined !== this.chunk) {
				// chunked response
				while (value) {
					if ("number" === typeof this.line) {
						// skip CR/LF at end of last chunk length
						const skip = (this.line < value) ? this.line : value;
						socket.read(null, skip);
						this.line -= skip;
						value -= skip;
						if (this.line)
							break;
						delete this.line;
						continue;
					}
					if (0 === this.chunk) {
						let line = socket.read(String, "\n");
						if (this.line) {
							line = this.line + line;
							delete this.line;
						}
						if (line[line.length - 1] !== "\n") {
							this.line = line;
							this.chunk = 0;
							break;
						}
						line = line.trim();		// removing training CR/LF etc.
						this.chunk = parseInt(line, 16);
						if (0 === this.chunk) {
							delete this.chunk;
							done.call(this);
							break;
						}
						value = socket.read();
					}

					let count = (this.chunk < value) ? this.chunk : value;
					if (count) {
						if (this.response) {
							this.buffers.push(socket.read(this.response, count));
							this.chunk -= count;
						}
						else {
							this.callback(Request.responseFragment, count);
							if (this.state >= 11)
								return;		// callback closed instance
							this.read(null);		// skip whatever the callback didn't read
						}
					}

					value = socket.read();

					if (0 === this.chunk)
						this.line = 2;	// should be two more bytes to read (CR/LF)... skip them when they become available
				}
			}
			else if (undefined !== this.total) {
				// content length
				const count = (this.total < value) ? this.total : value;
				if (this.response) {
					this.buffers.push(socket.read(this.response, count));
					this.total -= count - socket.read();
					if (0 === this.total) {
						delete this.total;
						done.call(this);
					}
				}
				else
					this.callback(Request.responseFragment, count);
			}
			else {
				// read until connection closed
				if (this.response)
					this.buffers.push(socket.read(this.response));
				else
					this.callback(Request.responseFragment, socket.read());
			}
		}
	}

	if (message < 0) {		// disconnected (or other error)
		let error = true;
		if (5 === this.state) {
			if (0 === this.total)
				error = false;
			else if ((undefined === this.total) && (undefined === this.chunk))	// end of data - no content-length and no chunk
				error = false;
		}
		done.call(this, error, value);
	}
}

function done(error = false, data) {
	if (6 === this.state) return;

	this.state = 6;
	delete this.done;

	if (this.response && !error) {
		if (String === this.response)
			data = this.buffers.join("");
		else if (ArrayBuffer === this.response) {
			if (0 === this.buffers.length)
				data = new ArrayBuffer();
			else if (1 === this.buffers.length)
				data = this.buffers[0];
			else {
				data = this.buffers.shift();
				data = data.concat.apply(data, this.buffers);
			}
		}
		delete this.buffers;
		delete this.response;
	}

	try {
		if (this.server) {
			if (error)
				this.callback(Server.error, data);
		}
		else
			this.callback(error ? Request.error : Request.responseComplete, data);
	}
	finally {
		this.close();
	}
}

/*
	Server

	state:
	
		0 - connecting
		1 - receiving request status
		2 - receiving request headers
		
		
		7 - sending response headers
		8 - sending response body
		9 - done
		
	callback values:
	
		-1 - disconnected
//@@		0 - get request body fragment
		1 - new connection request receieved
		2 - status received (path, method)
		3 - one header received (name, value)
		4 - all headers received (return request body type)
		5 - request body fragment (buffer)
		6 - request body complete (joined response...)

		7 - prepare response
		8 - sending response body

		9 - Get response fragment
		10 - Request complete

*/

export class Server {
	#listener;
	connections = [];

	constructor(dictionary = {}) {
		this.#listener = new Listener({port: 80, ...dictionary});
		this.#listener.callback = listener => {
			const socket = new Socket({listener: this.#listener, noDelay: true});
			const request = new Request({socket});	// request class will work to receive request body
			socket.callback = server.bind(request);
			request.server = this;					// associate server with request
			request.state = 1;						// already connected socket
			request.callback = this.callback;		// transfer server.callback to request.callback
			this.connections.push(request);
			request.callback(Server.connection);	// tell app we have a new connection
		};
	}

	close(connections = true) {
		if (connections) {
			this.connections.forEach(request => {
				try {
					request.close();
				}
				catch {
				}
			});
			delete this.connections;
		}
		this.#listener.close();
		this.#listener = undefined;
	}
	detach(connection) {
		const i = this.connections.indexOf(connection);
		if (i < 0) throw new Error;

		this.connections.splice(i, 1);

		const socket = connection.socket;
		delete socket.callback;
		connection.state = 10;
		delete connection.socket;
		connection.close();
		
		return socket;
	}
}
Server.connection = 1;
Server.status = 2;
Server.header = 3;
Server.headersComplete = 4;
Server.requestFragment = 5;
Server.requestComplete = 6;
Server.prepareResponse = 8;
Server.responseFragment = 9;
Server.responseComplete = 10;
Server.error = -1;

function server(message, value, etc) {
	let socket = this.socket;
	if (!socket) return;

//	trace(`HTTP SERVER SOCKET MSG ${message} in state ${this.state}\n`);

	try {
		if (2 === message) {
			if ((1 == this.state) || (2 == this.state)) {
				while (true) {
					let line = socket.read(String, "\n");
					if (!line)
						return;			// out of data. wait for more.

					if (this.line) {
						line = this.line + line;
						delete this.line;
					}

					if (10 != line.charCodeAt(line.length - 1)) {		// partial header line, accumulate and wait for more
						this.line = line;
						return;
					}

					if ("\r\n" == line) {		// empty line is end of headers
						delete this.line;

						let request = this.callback(Server.headersComplete);		// headers complete... let's see what to do with the request body
						if (false === request)
							delete this.total;				// ignore request body and just send response

						if (undefined !== this.total) {
							// start to receive request body
							this.state = 5;

							if (undefined !== request)
								this.request = request;

							if (true !== this.request)
								this.buffers = [];

							value = socket.read();
						}
						else {
							this.state = 7;		// send response headers
							message = 3;
							value = socket.write();
						}
						break;
					}

					if (1 === this.state) {
						// parse status line: GET / HTTP/1.1
						line = line.split(" ");
						if (line.length < 3)
							throw new Error("unexpected status format");
						const protocol = line[line.length - 1].trim();
						if (("HTTP/1.1" !== protocol) && ("HTTP/1.0" !== protocol))		// http 1.0 for hotspot.html
							throw new Error("bad protocol ID");

						line.length -= 1;		// remove "HTTP/1.1"
						let method = line.shift();
						let path = line.join(" ");	// re-aassemble path
						this.callback(Server.status, path, method);
						if (!this.socket)
							return;

						this.total = undefined;
						this.state = 2;
					}
					else if (2 === this.state) {
						let position = line.indexOf(":");
						let name = line.substring(0, position).trim().toLowerCase();
						let data = line.substring(position + 1).trim();

						if ("content-length" === name) {
							let length = parseInt(data);
							if (length)
								this.total = length;
						}
						if ("transfer-encoding" === name) {
							if ("chunked" === data.toLowerCase())
								throw new Error("chunked request body not implemented");
						}

						this.callback(Server.header, name, data);
					}
				}
			}
			if (5 === this.state) {		// fragment of request body
				let count = (value < this.total) ? value : this.total;
				if (0 === count) return;

				if (true === this.request)
					this.callback(Server.requestFragment, socket.read());	// callback reads the data
				else {
					this.buffers.push(socket.read(this.request, count));		// http server reads the data
					this.total -= count;
				}

				if (0 === this.total) {		// received complete request body
					let data;
					if (String === this.request)
						data = this.buffers.join("");
					else if (ArrayBuffer === this.request) {
						if (0 === this.buffers.length)
							data = new ArrayBuffer();
						else if (1 === this.buffers.length)
							data = this.buffers[0];
						else {
							data = this.buffers.shift();
							data = data.concat.apply(data, this.buffers);
						}
					}

					delete this.buffers;
					delete this.request;
					delete this.total;

					this.state = 7;

					this.callback(Server.requestComplete, data);		// end of request body

					message = 3;
					value = socket.write();
				}
			}
		}

		if (3 === message) {		// space available to write
			let first;

			if (7 === this.state) {
				const response = this.callback(Server.prepareResponse);		// prepare response

				const status = response?.status ?? 200;
				const message = response?.reason?.toString() ?? reason(status);
				const parts = ["HTTP/1.1 ", status.toString(), " ", message, "\r\n",
							"connection: ", "close\r\n"];

				if (response) {
					let byteLength;

					for (let i = 0, headers = response.headers; headers && (i < headers.length); i += 2) {
						parts.push(headers[i], ": ", headers[i + 1].toString(), "\r\n");
						if ("content-length" == headers[i].toLowerCase())
							byteLength = parseInt(headers[i + 1]);
					}

					this.body = response.body;
					if (true === response.body) {
						if (undefined === byteLength) {
							this.flags = 2;
							parts.push("transfer-encoding: chunked\r\n");
						}
						else
							this.flags = 4;
					}
					else {
						this.flags = 1;
						let count = 0;
						if (this.body)
							count = ("string" === typeof this.body) ? this.body.length : this.body.byteLength;	//@@ utf-8 hell
						parts.push("content-length: ", count.toString(), "\r\n");
					}
				}
				else
					parts.push("content-length: 0\r\n");
				parts.push("\r\n");
				socket.write.apply(socket, parts);

				this.state = 8;
				first = true;

				if (this.body && (true !== this.body)) {
					let count = ("string" === typeof this.body) ? this.body.length : this.body.byteLength;
					if (count > (socket.write() - ((2 & this.flags) ? 8 : 0)))
						return;
				}
			}
			if (8 === this.state) {
				let body = this.body;
				if (true === body)
					body = this.callback(Server.responseFragment, socket.write() - ((2 & this.flags) ? 8 : 0));		// account for chunk overhead

				let count = 0;
				if (undefined !== body)
					count = ("string" === typeof body) ? body.length : body.byteLength;

				if (0 === count) {
					if (!first)
						this.state = 9;		// done
				}
				else {
					if (1 & this.flags) {
						socket.write(body);				//@@ assume it all fits... not always true
						delete this.body;			// one shot
						this.state = 10;
					}
					else if (2 & this.flags) {
						socket.write(count.toString(16).toUpperCase(), "\r\n", body, "\r\n");
					}
					else if (4 & this.flags)
						socket.write(body);
				}
			}
			if (9 === this.state) {
				this.state = 10;
				if (2 & this.flags)
					socket.write("0\r\n\r\n");
				return;
			}
			if (10 === this.state) {
				try {
					this.callback(Server.responseComplete);
				}
				finally {
					this.server.connections.splice(this.server.connections.indexOf(this), 1);
					this.close();
				}
			}
		}
	}

	catch (e) {
		message = -1;			// close connection on error
	}

	if (-1 === message) {		// disconnected
		try {
			this.callback((10 === this.state) ? Server.responseComplete : Server.error);
		}
		finally {
			this.server.connections.splice(this.server.connections.indexOf(this), 1);
			this.close();
		}
	}
}

function reason(status)
{
	const message = `
100 Continue
101 Switching Protocols
200 OK
201 Created
202 Accepted
203 Non-Authoritative Information
204 No Content
205 Reset Content
206 Partial Content
207 Multi-Status
300 Multiple Choices
301 Moved Permanently
302 Found
303 See Other
304 Not Modified
305 Use Proxy
307 Temporary Redirect
400 Bad Request
401 Unauthorized
402 Payment Required
403 Forbidden
404 Not Found
405 Method Not Allowed
406 Not Acceptable
407 Proxy Authentication Required
408 Request Timeout
409 Conflict
410 Gone
411 Length Required
412 Precondition Failed
413 Request Entity Too Large
414 Request-URI Too Long
415 Unsupported Media Type
416 Requested Range Not Satisfiable
417 Expectation Failed
500 Internal Server Error
501 Not Implemented
502 Bad Gateway
503 Service Unavailable
504 Gateway Timeout
505 HTTP Version Not Supported
`;
	let index = message.indexOf(`\n${status} `);
	if (index < 0) return "OK";
	return message.substring(index + 5, message.indexOf("\n", index + 1));
}

export default Object.freeze({
	Request,
	Server
});
