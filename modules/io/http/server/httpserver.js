/*
 * Copyright (c) 2021-2026  Moddable Tech, Inc.
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
 
import Timer from "timer";

const more = Object.freeze({more: true});

class Connection {
	#socket;
	#from;
	#readable;
	#writable = 0;
	#writePosition;
	#pendingWrite;
	#line = "";
	#lineBuffer = new Uint8Array(new ArrayBuffer(64, {maxByteLength: 8192}));
	#state = "receiveRequest";
	#remaining;
	#chunk;
	#notify;
	#callbacks;
	#registered;
	#route;
	#current;
	#timer;
	#timeout;
	#keepAlive;
	#idle;

	constructor(from, done, keepAlive) {
		this.#from = from;
		this.#notify = done;
		this.#timeout = keepAlive ?? 0;
		this.#lineBuffer.position = 0;
	}
	close() {
		this.#notify?.(this);
		this.#notify = this.#callbacks = this.#registered = undefined;
		this.#socket?.close();
		this.#socket = undefined;
		this.#from?.close();
		this.#from = undefined;
		Timer.clear(this.#timer);
		this.#timer = undefined;
		Timer.clear(this.#idle);
		this.#idle = undefined;
	}
	accept(options) {
		const from = this.#from;
		this.#registered = this.#callbacks = {
			onRequest: options.onRequest,
			onReadable: options.onReadable,
			onResponse: options.onResponse,
			onWritable: options.onWritable,
			onDone: options.onDone,
			onError: options.onError
		};

		this.#socket = new from.constructor({
			from,
			onReadable: count => this.#onReadable(count),
			onWritable: count => this.#onWritable(count),
			onError: () => this.#onError("socket error")
		});
		this.#from = undefined;

		this.#armIdle();
	}
	#armIdle() {
		if (this.#timeout) {
			this.#idle ??= Timer.set(() => this.close(), this.#timeout);
			Timer.schedule(this.#idle, this.#timeout);
		}
	}
	detach() {
		const result = this.#socket ?? this.#from;
		if (!result)
			throw new Error;
		this.#socket = this.#from = undefined;

		this.#notify?.(this);
		this.#notify = undefined;

		return new result.constructor({from: result});
	}
	read(count) {
		if ("receiveBody" !== this.#state)
			throw new Error("bad state");

		const available = Math.min(this.#readable, (undefined === this.#chunk) ? this.#remaining : this.#chunk);
		if ((count > available) || (undefined === count))
			count = available;
		if (!count)
			return;

		this.#readable -= count;
		if (undefined === this.#chunk)
			this.#remaining -= count;
		else
			this.#chunk -= count;

		const result = this.#socket.read(count);

		if (0 === this.#chunk) {
			this.#resetLine();
			if (this.#readable) {
				this.#timer = Timer.set(() => {
					this.#timer = undefined;
					this.#onReadable(this.#readable);
				});
			}
		}
		else if (0 === this.#remaining)
			this.#timer = Timer.set(() => this.#reply());

		return result;
	}
	write(data) {
		if ("sendResponseBody" !== this.#state)
			throw new Error("bad state");

		if (!data) {
			if ("number" === typeof this.#remaining)
				throw new Error("bad data");

			this.#pendingWrite = ArrayBuffer.fromString("0\r\n\r\n");
			this.#writePosition = 0;
			this.#remaining = 0;
			if (this.#writable >= this.#pendingWrite.byteLength) {
				this.#writable = this.#socket.write(this.#pendingWrite);
				this.#pendingWrite = undefined;
				this.#done();
			}
			else {
				this.#timer = Timer.set(() => {
					this.#timer = undefined;
					if (this.#writable)
						this.#onWritable(this.#writable);
				});
			}
			return 0;
		}

		const byteLength = data.byteLength;
		if (true === this.#remaining) {
			if ((byteLength + 8) > this.#writable)
				throw new Error("would block");

			this.#socket.write(ArrayBuffer.fromString(byteLength.toString(16) + "\r\n"), more);
			this.#socket.write(data, more);
			this.#writable = this.#socket.write(ArrayBuffer.fromString("\r\n"));

			return (this.#writable > 8) ? (this.#writable - 8) : 0;
		}
		else {
			if ((byteLength > this.#writable) || (byteLength > this.#remaining))
				throw new Error("would block");

			this.#writable = this.#socket.write(data);

			this.#remaining -= byteLength;
			if (0 === this.#remaining) {
				this.#done();
				this.#writable = 0;
			}

			return this.#writable;
		}
	}
	#onReadable(count) {
		this.#readable = count;

		if (!this.#state.startsWith("receive")) {
			Timer.clear(this.#idle);
			this.#idle = undefined;
			return;
		}
		this.#armIdle();

		while (this.#readable) {
			if (undefined !== this.#line) {
				this.#socket.format = "number";
				let lineBuffer = this.#lineBuffer, position = lineBuffer.position, length = lineBuffer.length;
				while (this.#readable--) {
					const c = this.#socket.read();
					if (position === length)
						lineBuffer.buffer.resize(length << 1);
					lineBuffer[position++] = c;
					if (10 === c)
						break;
				}
				lineBuffer.position = position;
				this.#socket.format = "buffer";

				if ((13 !== lineBuffer[position - 2]) || (10 !== lineBuffer[position - 1])) {
					this.#readable = 0;
					return;
				}
				this.#line = String.fromArrayBuffer(lineBuffer.buffer, 0, position);
			}

			switch (this.#state) {
				case "receiveRequest": {
					const status = this.#line.trim().split(" ");
					if ((status.length < 3) || ("HTTP/1.1" !== status[2])) {
						this.#onError("badly formed");
						return;
					}

					this.#current = {
						method: status[0],
						headers: new Map
					};
					const query = status[1].indexOf("?");
					if (query < 0) {
						this.#current.path = status[1];
						this.#current.query = "";
					}
					else {
						this.#current.path = status[1].slice(0, query);
						this.#current.query = status[1].slice(query + 1);
					}
					this.#resetLine();
					this.#state = "receiveHeader";
					} break;

				case "receiveHeader":
					if ("\r\n" !== this.#line) {
						const position = this.#line.indexOf(":");
						if (position < 0)
							return void this.#onError("badly formed");
						const name = this.#line.slice(0, position).trim().toLowerCase();
						let data = this.#line.slice(position + 1).trim();
						this.#current.headers.set(name, data);

						if ("content-length" === name) {
							this.#remaining = parseInt(data);
							if (!(this.#remaining >= 0))
								return void this.#onError("badly formed");
						}
						else if ("transfer-encoding" === name) {
							data = data.toLowerCase();
							if ("chunked" === data)
								this.#chunk = 0;
						}

						this.#resetLine();
					}
					else {
						if (undefined !== this.#chunk)
							this.#remaining = undefined;		// ignore content-length if chunked

						this.#keepAlive = ("close" === this.#current.headers.get("connection")?.toLowerCase()) ? false : this.#timeout;

						this.#callbacks.onRequest?.call(this, this.#current);
						if (!this.#socket)
							return;
						this.#current = undefined;

						if (!this.#remaining && (undefined == this.#chunk)) {
							this.#resetLine();
							this.#reply();
							return;
						}

						this.#state = "receiveBody";
						if (undefined == this.#chunk)
							this.#line = undefined;
						else
							this.#resetLine();
					}
					break;

				case "receiveBody": {
					let count = this.#remaining;
					if (undefined !== this.#chunk) {
						if (0 === this.#chunk) {
							if ("\r\n" === this.#line)
								continue;
							this.#chunk = parseInt(this.#line.trim(), 16);
							if (!(this.#chunk >= 0))
								return void this.#onError("badly formed");
							this.#line = undefined;

							if (0 === this.#chunk) {
								this.#state = "receiveChunkTrailer";
								this.#resetLine();
								continue;
							}
						}
						count = this.#chunk;
					}

					if (this.#callbacks.onReadable) {
						count = Math.min(this.#readable, count);
						if (count)
							this.#callbacks.onReadable.call(this, count);
					}
					else
						this.read();
					}
					return;

				case "receiveChunkTrailer":
					if ("\r\n" !== this.#line)
						this.#onError("badly formed");
					else
						this.#reply();
					return;

				default:
					this.#onError("bad state");
					return;
			}
		}
	}
	#onWritable(count) {
		this.#writable = count;

		if (this.#state.startsWith("receive"))
			return;		// initial on-writable

		while (this.#writable) {
			if (this.#pendingWrite) {
				const use = Math.min(this.#pendingWrite.byteLength - this.#writePosition, this.#writable);
				this.#writable = this.#socket.write(new Uint8Array(this.#pendingWrite, this.#writePosition, use));
				this.#writePosition += use;
				if (this.#writePosition !== this.#pendingWrite.byteLength)
					return;

				this.#pendingWrite = undefined;
			}

			switch (this.#state) {
				case "sendResponseHeader": {
					const item = this.#current.iterator.next();
					if (item.done) {
						this.#pendingWrite = "\r\n";
						this.#state = "sendResponseBody";
						if (101 === this.#current.status)
							this.#remaining = 0;
						this.#current = undefined;		// the response headers have been sent
					}
					else {
						const name = item.value[0];
						this.#pendingWrite = name + ": " + item.value[1] + "\r\n";
						if ("content-length" === name) {
							this.#remaining = parseInt(item.value[1]);
							if (!(this.#remaining >= 0))
								return void this.#onError("badly formed");
						}
						else if ("transfer-encoding" === name) {
							if ("chunked" === item.value[1])
								this.#remaining = true;
						}
					}
					this.#pendingWrite = ArrayBuffer.fromString(this.#pendingWrite);
					this.#writePosition = 0;
					} break;

				case "sendResponseBody": {
					if (0 === this.#remaining) {
						this.#done();
						return;
					}

					let writable = this.#writable;
					if (true === this.#remaining) {
						writable -= 8;
						if (writable <= 0)
							return;
					}
					else if ((undefined !== this.#remaining) && (writable > this.#remaining))
						writable = this.#remaining;
					this.#callbacks.onWritable?.call(this, writable);
					return;
					}

				case "waitResponse":
				case "done":
					return;

				default:
					this.#onError("bad state");
					return;
			}
		}
	}
	#onError(msg) {
		const onError = this.#callbacks?.onError;
		this.#state = "error";
		this.close();
		onError?.call(this, msg);
	}
	#done() {
		this.#state = "done";
		try {
			this.#callbacks?.onDone?.call(this);
		}
		catch {
			/* this space intentionally left blank */
		}
		if (this.#keepAlive && this.#socket)
			this.#next();
		else
			this.close();
	}
	#next() {
		this.#route = undefined;
		this.#callbacks = this.#registered;
		this.#current = undefined;
		this.#state = "receiveRequest";
		this.#resetLine();
		this.#remaining = this.#chunk = undefined;
		this.#pendingWrite = this.#writePosition = undefined;
		Timer.clear(this.#timer);
		this.#timer = undefined;
		if (this.#readable) {
			this.#timer = Timer.set(() => {
				this.#timer = undefined;
				this.#onReadable(this.#readable);
			});
		}
		else
			this.#armIdle();
	}
	#reply() {		// request headers & request body received. time to reply.
		this.#timer = undefined;
		Timer.clear(this.#idle);
		this.#idle = undefined;
		this.#state = "waitResponse";

		const response = {
			headers: new Map,
			status: 200
		};
		const onResponse = this.#callbacks.onResponse;
		if (onResponse)
			onResponse.call(this, response);
		else
			this.respond(response);
	}
	respond(response) {
		this.#state = "sendResponseHeader";

		const connection = response.headers.get("connection");
		if (this.#keepAlive) {
			if (("close" === connection) || (101 === response.status) ||
				!(response.headers.has("content-length") || ("chunked" === response.headers.get("transfer-encoding"))))
				this.#keepAlive = false;
		}

		let pendingWrite = `HTTP/1.1 ${response.status} ${reason(response.status)}\r\n`;
		if (!connection && !this.#keepAlive)
			pendingWrite += "connection: close\r\n";
		this.#pendingWrite = ArrayBuffer.fromString(pendingWrite);
		this.#writePosition = 0;

		response.iterator = response.headers.entries();
		this.#current = response;
		this.#remaining = undefined;

		this.#onWritable(this.#writable);
	}
	get route() {
		return this.#route;
	}
	set route(route) {
		if ("receiveHeader" !== this.#state)
			throw new Error("bad state");

		this.#route = route;
		this.#callbacks = {
			onRequest: route?.onRequest,
			onReadable: route?.onReadable,
			onResponse: route?.onResponse,
			onWritable: route?.onWritable,
			onDone: route?.onDone,
			onError: route?.onError
		};

		this.#callbacks.onRequest?.call(this, this.#current);
		this.#current = undefined;
	}
	#resetLine() {
		this.#line = "";
		this.#lineBuffer.buffer.resize(64);
		this.#lineBuffer.position = 0;
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

class HTTPServer {
	#onConnect;
	#listener;
	#connections = new Set;
	#onRoute;
	#keepAlive;

	constructor(options) {
		this.#onConnect = options.onConnect;
		this.#onRoute = options.onRoute;
		this.#keepAlive = options.keepAlive ?? 10_000;
		if (!this.#onConnect === !this.#onRoute)
			throw new Error("invalid");
		const target = options.target;
		if (undefined !== target)
			this.target = target;

		this.#listener = new options.socket.io({
			...options.socket,
			port: options.port ?? 80,
			target: this,
			onReadable() {
				let from;
				while ((from = this.read())) {
					const connection = new Connection(from, connection => this.target.#connections?.delete(connection), this.target.#keepAlive);
					this.target.#connections.add(connection);
					if (this.target.#onRoute) {
						try {
							connection.accept({
								onRequest: request => {
									try {
										connection.route = this.target.#onRoute.call(this.target, request) || {
											onResponse(response) {
												response.status = 404;
												response.headers.set("content-length", 0);
												this.respond(response);
											}
										};
									}
									catch {
										connection.close();
									}
								}
							});
						}
						catch {
							connection.close();		// the connection was reset before it could be accepted
						}
					}
					else {
						try {
							this.target.#onConnect(connection);
						}
						catch {
							connection.close();
						}
					}
				}
			}
		});
	}
	close() {
		this.#connections?.forEach(connection => connection.close());
		this.#connections = undefined;
		this.#listener?.close();
		this.#listener = undefined;
	}
	get port() {
		return this.#listener.port;
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

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
507 Insufficient Storage
`;

function reason(status)
{
	const index = message.indexOf(`\n${status} `);
	if (index < 0) return "";
	return message.slice(index + 5, message.indexOf("\n", index + 1));
}

export default HTTPServer;
