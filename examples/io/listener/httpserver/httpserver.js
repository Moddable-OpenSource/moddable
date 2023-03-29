/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
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

class Connection {
	#server;
	#socket;
	#from;
	#readable;
	#writable = 0;
	#writePosition;
	#pendingWrite;
	#line = "";
	#state = "receiveRequest";
	#remaining;
	#chunk;
	#options = {};
	#route;
	#timer;

	constructor(server, from, done) {
		this.#server = server;
		this.#from = from;
		this.#options.done = done;			
	}
	close() {
		this.#options?.done?.(this);
		this.#options = undefined;
		this.#socket?.close();
		this.#socket = undefined; 
		this.#from?.close();
		this.#from = undefined;
		Timer.clear(this.#timer); 
		this.#timer = undefined;
	}
	accept(options) {
		const from = this.#from;
		this.#options.onRequest = options.onRequest; 
		this.#options.onReadable = options.onReadable; 
		this.#options.onResponse = options.onResponse; 
		this.#options.onWritable = options.onWritable; 
		this.#options.onDone = options.onDone 
		this.#options.onError = options.onError; 
		
		this.#socket = new from.constructor({
			from,
			onReadable: count => this.#onReadable(count),
			onWritable: count => this.#onWritable(count),
			onError: error => this.#onError(error)
		});
		this.#from = undefined;
	}
	detach() {
		const result = this.#socket ?? this.#from;
		if (!result)
			throw new Error;
		this.#socket = this.#from = undefined;

		this.#options?.done(this);
		delete this.#options.done;

		return new result.constructor({from: result});
	}
	read(count) {
		if ("receiveBody" !== this.#state)
			return;
					
		const available = Math.min(this.#readable, (undefined === this.#chunk) ? this.#remaining : this.#chunk);
		if (count > available)
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
			this.#line = "";
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
			if (true !== this.#remaining)
				throw new Error("bad data");

//@@ this may not be always correct... if last chunk has already flushed and onWritable called, this will never go out
			this.#pendingWrite = ArrayBuffer.fromString("0000\r\n\r\n");
			this.#remaining = 0;
			this.#timer = Timer.set(() => {
				this.#timer = undefined;
				if (this.#writable)
					this.#onWritable(this.#writable);
			});
			return 0; 
		}

		const byteLength = data.byteLength;
		if (true === this.#remaining) {
			if ((byteLength + 8) > this.#writable)
				throw new Error("too much");

			this.#writable -= byteLength + 8;
			this.#socket.write(ArrayBuffer.fromString(byteLength.toString(16).padStart(4, "0") + "\r\n"));
			this.#socket.write(data);
			this.#socket.write(ArrayBuffer.fromString("\r\n"));

			return (this.#writable > 8) ? (this.#writable - 8) : 0 
		}
		else {
			if ((byteLength > this.#writable) || (byteLength > this.#remaining))
				throw new Error("too much");

			this.#writable -= byteLength;
			this.#socket.write(data);

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

		while (this.#readable) {
			if (undefined !== this.#line) {
				this.#socket.format = "number";
				while (this.#readable--) {
					const c = this.#socket.read();
					this.#line += String.fromCharCode(c);
					if (10 === c)
						break;
				}
				this.#socket.format = "buffer";

				if (!this.#line.endsWith("\r\n"))
					return;
			}

			switch (this.#state) {
				case "receiveRequest": {
					const status = this.#line.trim().split(" ");
					if ((status.length < 3) || ("HTTP/1.1" !== status[2])) {
						this.#onError("badly formed");
						return;
					}

					this.#options.method = status[0];
					this.#options.path = status[1];
					this.#line = "";
					this.#state = "receiveHeader";
					this.#options.headers = new Map;
					} break;

				case "receiveHeader":
					if ("\r\n" !== this.#line) {
						const position = this.#line.indexOf(":");
						const name = this.#line.substring(0, position).trim().toLowerCase();
						let data = this.#line.substring(position + 1).trim();
						this.#options.headers.set(name, data);

						if ("content-length" === name)
							this.#remaining = parseInt(data);
						else if ("transfer-encoding" === name) {
							data = data.toLowerCase();
							if ("chunked" === data)
								this.#chunk = 0;
						}

						this.#line = "";
					}
					else {					
						if (undefined !== this.#chunk)
							this.#remaining = undefined;		// ignore content-length if chunked
							
						this.#options.onRequest?.call(this, {
							method: this.#options.method,
							path: this.#options.path,
							headers: this.#options.headers							
						});
						if (!this.#socket)
							return;
						delete this.#options.headers;
						delete this.#options.path;
						delete this.#options.method;

						if (!this.#remaining && (undefined == this.#chunk))
							this.#reply();
						else {
							this.#state = "receiveBody";
							this.#line = (undefined == this.#chunk) ? undefined : "";
						}
					}
					break;

				case "receiveBody":
					if (undefined !== this.#chunk) {
						if (0 === this.#chunk) {
							if ("\r\n" === this.#line)
								continue;
							this.#chunk = parseInt(this.#line.trim(), 16);
							this.#line = undefined;
							
							if (0 === this.#chunk) {
								this.#state = "receiveChunkTrailer";
								this.#line = "";
								continue;
							}
						}
						this.#options.onReadable?.call(this, Math.min(this.#readable, this.#chunk));
					}
					else
						this.#options.onReadable?.call(this, Math.min(this.#readable, this.#remaining));
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
				let use = this.#pendingWrite.byteLength - this.#writePosition;
				if (use > count) {
					this.#socket.write(new Uint8Array(this.#pendingWrite, this.#writePosition, count));
					this.#writePosition += count;
					this.#writable = 0;
					return;
				}
				
				this.#socket.write(this.#pendingWrite);
				this.#pendingWrite = undefined;
				this.#writable -= use;
			}

			switch (this.#state) {
				case "sendResponse":
					this.#pendingWrite = `HTTP/1.1 ${this.#options.status} FILL_THIS_IN\r\n`;
					this.#pendingWrite = ArrayBuffer.fromString(this.#pendingWrite);
					this.#writePosition = 0;

					this.#state = "sendResponseHeader";
					break;

				case "sendResponseHeader": {
					const item = this.#options.headers.next();
					if (item.done) {
						this.#pendingWrite = "\r\n";
						this.#state = "sendResponseBody";
						delete this.#options.headers;
					}
					else {
						const name = item.value[0];
						this.#pendingWrite = name + ": " + item.value[1] + "\r\n";
						if ("content-length" === name)
							this.#remaining = parseInt(item.value[1]);
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
					this.#options.onWritable?.call(this, writable);
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
		const onError = this.#options.onError; 
		this.#state = "error";
		this.close();
		onError?.call(this, msg);
	}
	#done() {
		this.#state = "done";
		try {
			this.#options.onDone?.call(this);
		}
		catch {
		}
		this.close();
	}
	#reply() {		// request headers & request body received. time to reply.
		this.#timer = undefined;
		this.#state = "waitResponse";

		const response = {
			headers: new Map,
			status: 200
		};
		if (this.#options.onResponse)
			this.#options.onResponse.call(this, response);
		else
			this.respond(response);
	}
	respond(response) {
		this.#state = "sendResponse";

		this.#options.status = response.status;
		this.#options.headers = response.headers.entries();
		this.#remaining = 0;

		this.#onWritable(this.#writable);
	}
	get route() {
		return this.#route;
	}
	set route(route) {
		this.#route = route;
		this.#options.onRequest = route.onRequest; 
		this.#options.onReadable = route.onReadable; 
		this.#options.onResponse = route.onResponse; 
		this.#options.onWritable = route.onWritable; 
		this.#options.onDone = route.onDone 
		this.#options.onError = route.onError; 
		if (this.#state === "receiveHeader") {
			this.#options.onRequest?.call(this, {
				method: this.#options.method,
				path: this.#options.path,
				headers: this.#options.headers							
			});
		}
		else {
			// error?
		}
	}
}

class HTTPServer {
	#onConnect;
	#listener;
	#connections = new Set;

	constructor(options) {
		this.#onConnect = options.onConnect;

		this.#listener = new options.io({
			port: options.port ?? 80,
			target: this,
			onReadable(count) {
				while (count--) {
					try {
						const connection = new Connection(this, this.read(), connection => this.target.#connections.delete(connection));
						this.target.#onConnect(connection);
					}
					catch {
						trace("igoring error!");
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
}

export default HTTPServer;
