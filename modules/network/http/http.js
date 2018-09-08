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
	http
*/

import {Socket, Listener} from "socket";

/*
	Client

	state:
	
		0 - connecting
		1 - sending request headers
		2 - sending request body
		3 - receiving status
		4 - receeving response headers
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
			this.socket = dictionary.socket;
			return;
		}

		// port, host, address, path (everything after port), method (default GET), headers, body, response
		this.method = dictionary.method ? dictionary.method : "GET";
		this.path = dictionary.path ? dictionary.path : "/";
		this.host = dictionary.host ? dictionary.host : dictionary.address;
		this.headers = dictionary.headers;
		this.body = dictionary.body;
		this.state = 0;
		this.response = dictionary.response;

		if (!dictionary.port) dictionary.port = 80;		//@@ maybe eliminate default port, require it to be handled by higher layers
		if (dictionary.Socket)
			this.socket = new dictionary.Socket(Object.assign({}, dictionary.Socket, dictionary));
		else
			this.socket = new Socket(dictionary);
		this.socket.callback = callback.bind(this);
	}

	read(type, limit) {
		if (undefined !== this.chunk) {
			if (undefined === limit)
				limit = this.chunk;
			else if (typeof limit !== "number")	//@@ number
				throw new Error("http limit only supports number");

			if (type === Number)
				limit = 1;
			else if (limit > this.chunk)
				limit = this.chunk;

			let read = this.socket.read();
			if (read < limit)
				limit = read;

			if (0 === limit)
				return;

			this.chunk -= limit;
		}

		return this.socket.read(type, limit);
	}

	close() {
		if (this.socket)
			this.socket.close();
		delete this.socket;
		delete this.buffers;
	}
};

function callback(message, value) {
	let socket = this.socket;

	if (1 === message) {	// connected
		if (0 !== this.state)
			throw new Error("socket connected but http not in connecting state");

		this.state = 1;		// advance to sending request headers state

		let parts = [];
		parts.push(this.method, " ", this.path, " HTTP/1.1\r\n");
			//@@ Date: would be nice here...
		parts.push("Host: ", this.host, "\r\n");
		parts.push("Connection: close\r\n");

		let length;
		for (let i = 0; i < (this.headers ? this.headers.length : 0); i += 2) {
			let name = this.headers[i].toString();
			parts.push(name, ": ", this.headers[i + 1].toString(), "\r\n");
			if ("content-length" === name.toLowerCase())
				length = true;
		}

		if (this.body && (true !== this.body) && !length) {
			let length = (this.body instanceof ArrayBuffer) ? this.body.byteLength : this.body.length;
			parts.push("content-length: ", length.toString(), "\r\n");
		}

		parts.push("\r\n");
		socket.write.apply(socket, parts);

		delete this.method;
		delete this.path;
		delete this.host;
		delete this.headers;

		if (undefined === this.body) {
			this.state = 3;		// advance to receiving status state
			return;
		}

		this.state = 2;			// begin to transmit request body immediately, if possbile
		message = 3;
	}

	if (3 === message) {	// safe to write more data
		if (2 === this.state) {
			if (true === this.body) {
				let body = this.callback(0, socket.write());
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
				if (socket.read())
					throw new Error("didn't receive full status line!?!");

				this.line = status;
				return;
			}

			// parse status line:  HTTP/1.1 200 OK
			status = status.split(" ");
			if (status.length < 3)
				throw new Error("unexpected status format");
			this.callback(1, parseInt(status[1]));

			this.state = 4;			// advance to receiving response headers state
			this.line = undefined;	// no header line fragment yet

			this.total = undefined;		// total number of bytes expected (content-length)
			this.chunk = undefined;		// bytes remaining in this chunk (undefined if not chunked)
		}

		if (4 === this.state) {		// receiving response headers
			while (true) {
				let line = socket.read(String, "\n");
				if (!line)
					return;			// out of data. wait for more.

				if (this.line) {
					line = this.line + line;
					this.line = undefined;
				}

				if (10 !== line.charCodeAt(line.length - 1)) {		// partial header line, accumulate and wait for more
					this.line = line;
					return;
				}

				if ("\r\n" === line) {		// empty line is end of headers
					this.callback(3);		// all response headers received
					delete this.line;
					this.state = 5;			// advance to receiving response headers state

					if (this.response)
						this.buffers = [];	// array to hold response fragments

					value = socket.read();	// number of bytes available
					if (0 == value)
						return;

					break;
				}

				let position = line.indexOf(":");
				let name = line.substring(0, position).trim().toLowerCase();
				let data = line.substring(position + 1).trim();
				this.callback(2, name, data);		// one received header

				if ("content-length" === name)
					this.total = parseInt(data);
				else if ("transfer-encoding" === name) {
					if ("chunked" === data.toLowerCase())
						this.chunk = 0;
				}
			}
		}

		if (5 === this.state) {		// receiving response body
			if (undefined !== this.chunk) {
				// chunked response
				while (value) {
					if (0 === this.chunk) {
						// read chunk length @@ will fail if chunk length spans two read buffers
						let line = socket.read(String, "\n");
						line = line.trim();		// removing training CR/LF etc.
						this.chunk = parseInt(line, 16);
						if (0 === this.chunk) {
							delete this.chunk;
							done.call(this);
							break;
						}
						value = socket.read();
					}

					let count = Math.min(this.chunk, value);
					if (this.response) {
						this.buffers.push(socket.read(this.response, count));
						this.chunk -= count;
					}
					else {
						this.callback(4, count);
						this.read(null);		// skip whatever the callback didn't read
					}

					value = socket.read();

					if (0 === this.chunk) {
						// should be two more bytes to read (CR/LF).... @@ need to be in read buffer... or will fail
						if (13 !== socket.read(Number))
							throw new Error("expected CR");
						if (10 !== socket.read(Number))
							throw new Error("expected LF");

						value -= 2;
					}
				}
			}
			else if (undefined !== this.total) {
				// content length
				let count = Math.min(this.total, value);
				this.total -= count;
				if (this.response)
					this.buffers.push(socket.read(this.response, count));
				else
					this.callback(4, count);
				if (0 === this.total) {
					delete this.total;
					done.call(this);
				}
			}
			else {
				// read until connection closed
				if (this.response)
					this.buffers.push(socket.read(this.response));
				else
					this.callback(4, socket.read());
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
		done.call(this, error);
	}
}

function done(error = false) {
	let data;

	if (6 == this.state) return;

	this.state = 6;

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

	this.callback(error ? -2 : 5, data);
	this.close();
}

/*
	Server

	state:
	
		0 - connecting
		1 - receieving request status
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
	constructor(dictionary) {
		this.listener = new Listener({port: dictionary.port ? dictionary.port : 80});
		this.listener.callback = listener => {
			let socket = new Socket({listener: this.listener});
			let request = new Request({socket});	// request class will work to receive request body
			socket.callback = server.bind(request);
			request.server = this;					// associate server with request
			request.state = 1;						// already connected socket
			request.callback = this.callback;		// transfer server.callback to request.callback
			request.callback(1);					// tell app we have a new connection
		};
	}

	close() {
		this.listener.close();
		delete this.listener;
	}
}

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
						this.line = undefined;
					}

					if (10 != line.charCodeAt(line.length - 1)) {		// partial header line, accumulate and wait for more
	trace("partial header!!\n");		//@@ untested
						this.line = line;
						return;
					}

					if ("\r\n" == line) {		// empty line is end of headers
						delete this.line;

						let request = this.callback(4);		// headers complete... let's see what to do with the request body
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
						if (("HTTP/1.1" != protocol) && ("HTTP/1.0" != protocol))		// http 1.0 for hotspot.html
							throw new Error("bad protocol ID");

						line.length -= 1;		// remove "HTTP/1.1"
						let method = line.shift();
						let path = line.join(" ");	// re-aassemble path
						this.callback(2, path, method);

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

						this.callback(3, name, data);
					}
				}
			}
			if (5 === this.state) {		// fragment of request body
				let count = Math.min(value, this.total);
				if (0 === count) return;
				this.total -= count;

				if (true === this.request)
					this.callback(5, socket.read());	// callback reads the data
				else
					this.buffers.push(socket.read(this.request, count));		// http server reads the data

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

					this.callback(6, data);		// end of request body

					message = 3;
					value = socket.write();
				}
			}
		}

		if (3 === message) {		// space available to write
			if (7 === this.state) {
				let response = this.callback(8);		// prepare response

				let parts = [];
				let status = (!response || (undefined === response.status)) ? 200 : response.status;
				let message = (!response || (undefined === response.reason)) ? reason(status) : response.reason.toString();
				parts.push("HTTP/1.1 ", status.toString(), " ", message, "\r\n",
							"connection: ", "close\r\n");

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
						this.offset = 0;		// byte offset into this.body
					}
				}
				else
					parts.push("content-length: 0\r\n");
				parts.push("\r\n");
				socket.write.apply(socket, parts);

				this.state = 8;

				if (this.body && (true !== this.body)) {
					let count = ("string" === typeof this.body) ? this.body.length : this.body.byteLength;
					if (count > (socket.write() - ((2 & this.flags) ? 8 : 0)))
						return;
				}
			}
			if (8 === this.state) {
				let body = this.body;
				if (true === body)
					body = this.callback(9, socket.write() - ((2 & this.flags) ? 8 : 0));		// account for chunk overhead

				let count = 0;
				if (undefined !== body)
					count = ("string" === typeof body) ? body.length : body.byteLength;

				if (0 === count)
					this.state = 9;		// done
				else {
					if (1 & this.flags) {
						socket.write(body);				//@@ assume it all fits... not always true
						this.body = undefined;			// one shot
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
				if (2 & this.flags) {
					socket.write("0\r\n\r\n");
					return;
				}
			}
			if (10 === this.state) {
				this.callback(10);
				this.close();
				this.state = 11;
			}
		}
	}

	catch (e) {
		message = -1;			// close connection on error
	}

	if (-1 === message) {		// disconnected
		this.close();
		this.state = 11;
		this.callback(-1);
	}
}

function reason(status)
{
	let message = `
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

export default {
	Request,
	Server
};
