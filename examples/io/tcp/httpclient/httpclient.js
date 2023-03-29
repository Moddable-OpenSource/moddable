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

class HTTPClient {
	static #Request = class {
		#client;
		
		constructor(client) {
			this.#client = client;
		}
		read(count = this.#client.#readable) {
			const client = this.#client;
			if (client.#current.request !== this)
				throw new Error("bad state");
			
			if ("receiveBody" !== client.#state)
				return undefined;

			let buffer;
			if ("object" === typeof count) {
				buffer = count;
				count = buffer.byteLength;
			}			
			const available = Math.min(client.#readable, (undefined === client.#chunk) ? client.#remaining : client.#chunk);
			if (count > available) {
				count = available;
				if (buffer) {
					if (buffer.BYTES_PER_ELEMENT > 1)		// allows ArrayBuffer, SharedArrayBuffer, Uint8Array, Int8Array, DataView. disallows multi-byte element arrays.
						throw new Error("invalid buffer");
					if (ArrayBuffer.isView(buffer))
						buffer = new Uint8Array(buffer.buffer, buffer.byteOffset, count);
					else
						buffer = new Uint8Array(buffer, 0, count);
				}
			}

			client.#readable -= count;
			if (undefined === client.#chunk)
				client.#remaining -= count;
			else
				client.#chunk -= count;

			const result = client.#socket.read(buffer ?? count);

			if (0 === client.#chunk) {
				client.#line = "";
				if (client.#readable) {
					client.#timer = Timer.set(() => {
						client.#timer = undefined;
						client.#onReadable(client.#readable);
					});
				}
			}
			else if (0 === client.#remaining) {
				client.#state = "receivedBody"; 
				client.#timer = Timer.set(() => client.#done());
			}

			return result;
		}
		write(data) {
			const client = this.#client;
			if (client.#current.request !== this)
				throw new Error("bad state");

			if ("sendRequestBody" !== client.#state)
				throw new Error("bad state");

			if (!data) {
				if (true !== client.#requestBody)
					throw new Error("bad data");

//@@ this may not be always correct... if last chunk has already flushed and onWritable called, this will never go out
				client.#pendingWrite = ArrayBuffer.fromString("0000\r\n\r\n");
				client.#requestBody = false;
				return 0;		// request done. can't write more. 
			}

			const byteLength = data.byteLength;
			if (true === client.#requestBody) {
				if ((byteLength + 8) > client.#writable)
					throw new Error("too much");

				client.#writable -= byteLength + 8;
				client.#socket.write(ArrayBuffer.fromString(byteLength.toString(16).padStart(4, "0") + "\r\n"));
				client.#socket.write(data);
				client.#socket.write(ArrayBuffer.fromString("\r\n"));

				return (client.#writable > 8) ? (client.#writable - 8) : 0 
			}
			else {
				if ((byteLength > client.#writable) || (byteLength > client.#requestBody))
					throw new Error("too much");

				client.#writable -= byteLength;
				client.#socket.write(data);

				client.#requestBody -= byteLength;
				if (0 === client.#requestBody) {
					client.#state = "receiveResponseStatus";
					client.#line = "";
					client.#requestBody = false;
				}

				return client.#writable;
			}
		}
	}
	#socket;
	#current;
	#requests = [];
	#host;
	#state = "disconnected";
	#writable = 0;
	#readable = 0;
	#pendingWrite;
	#writePosition;
	#headers;
	#line;
	#status;
	#remaining;
	#requestBody;
	#chunk;
	#timer;
	#onError;
	
	constructor(options) {
		const {host, port, onError} = options; 

		if (!host) throw new Error("host required");
		this.#host = host;
		this.#onError = onError;

		const dns = new options.dns.io(options.dns);
		dns.resolve({
			host: this.#host, 

			onResolved: (host, address) => {
				try {
					this.#socket = new options.socket.io({
						...options.socket,
						address,
						host,
						port: port ?? 80,
						onReadable: count => this.#onReadable(count),
						onWritable: count => this.#onWritable(count),
						onError: () => this.#error()
					});
				}
				catch (e) {
					this.#state = "error";
					this.#error?.(e);
				}
			},
			onError: e => {
				this.#state = "error";
				this.#error?.(e);
			}
		});
	}
	close() {
		this.#socket?.close();
		this.#socket = undefined;
		Timer.clear(this.#timer);
		this.#timer = undefined;
		this.#current = undefined;
		this.#requests.length = 0;
		this.#state = "closed";
	}
	request(options) {
		options = {...options};
		this.#requests.push(options);
		if (("connected" === this.#state) && (1 === this.#requests.length)) {
			this.#next();
			this.#timer = Timer.set(() => {
				this.#timer = undefined;
				this.#onWritable(this.#writable);
			});
		}
		options.request = new HTTPClient.#Request(this); 
		return options.request;
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
				case "receiveResponseStatus": {
					const status = this.#line.split(" ");
					if (status.length < 3)
						throw new Error("");
					this.#status = parseInt(status[1]);
					this.#line = "";
					this.#state = "receiveHeader";
					this.#headers = new Map;
					} break;

				case "receiveHeader":
					if ("\r\n" !== this.#line) {
						const position = this.#line.indexOf(":");
						const name = this.#line.substring(0, position).trim().toLowerCase();
						let data = this.#line.substring(position + 1).trim();
						this.#headers.set(name, data);

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
						else if (undefined === this.#remaining)
							this.#remaining = Infinity;
							
						this.#current.onHeaders?.call(this.#current.request, this.#status, this.#headers);
						if (!this.#current) return;			// closed in callback

						this.#headers = undefined;
						this.#state = "receiveBody";
						this.#line = (undefined == this.#chunk) ? undefined : "";
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
						this.#current.onReadable?.call(this.#current.request, Math.min(this.#readable, this.#chunk));
					}
					else
						this.#current.onReadable?.call(this.#current.request, Math.min(this.#readable, this.#remaining));
					return;
				
				case "receiveChunkTrailer":
					if ("\r\n" !== this.#line)
						throw new Error;

					this.#done();
					return;

				default:
					throw new Error;		//@@ unexpected
			}
		}
	}
	#onWritable(count) {
		this.#writable = count;

		do {
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
				case "disconnected":
					this.#state = "connected";
					this.#next();
					if ("sendRequest" !== this.#state)
						break;
				
				case "sendRequest":
					this.#pendingWrite = (this.#current.method ?? "GET") + " " + (this.#current.path || "/") + " HTTP/1.1\r\n";
					this.#pendingWrite += "host: " + this.#host + "\r\n";
					this.#pendingWrite = ArrayBuffer.fromString(this.#pendingWrite);
					this.#writePosition = 0;

					this.#state = "sendRequestHeader";
					break;

				case "sendRequestHeader": {
					const item = this.#headers.next();
					if (item.done) {
						this.#pendingWrite = "\r\n";
						this.#state = "sendRequestBody";
						this.#headers = undefined;
					}
					else {
						const name = item.value[0];
						this.#pendingWrite = name + ": " + item.value[1] + "\r\n";
						if ("content-length" === name)
							this.#requestBody = parseInt(item.value[1]);
						else if ("transfer-encoding" === name) {
							if ("chunked" === item.value[1])
								this.#requestBody = true;
						}
					}
					this.#pendingWrite = ArrayBuffer.fromString(this.#pendingWrite);
					this.#writePosition = 0;
					} break;

				case "sendRequestBody":
					if (this.#requestBody) {
						let writable = this.#writable;
						if (true === this.#requestBody) {
							writable -= 8;
							if (writable <= 0)
								return;
						}
						this.#current.onWritable?.call(this.#current.request, writable);
					}
					else {
						this.#state = "receiveResponseStatus";
						this.#line = "";
					}
					break;
				
				case "closed":
					return;
			}
			
			if (!this.#pendingWrite || !this.#writable)
				break;
		} while (true);
	}
	#error(e) {
		if (("receivedBody" === this.#state) && this.#timer) {		// completion not reported yet. report before handling error.
			Timer.clear(this.#timer);
			this.#done();
		}

		this.#state = "error";

		try {
			const current = this.#current;
			this.#current = undefined;
			current?.onDone?.call(current.request, new Error);

			while (this.#requests.length) {
				const request = this.#requests.shift(); 
				request.onDone?.call(request.request, new Error);
			}
			this.#onError?.(e);
		}
		catch {
		}
		this.close();
	}
	#done() {
		this.#timer = undefined;

		this.#state = "connected";
		this.#current.onDone?.call(this.#current.request, null);
		this.#next();
		if (this.#current)
			this.#onWritable(this.#writable);
	}
	#next() {
		this.#current = this.#requests.shift();
		if (!this.#current)
			return;

		this.#line = undefined;
		this.#pendingWrite = undefined;
		this.#state = "sendRequest";
		this.#headers = (this.#current.headers ?? new Map).entries();
		this.#remaining = undefined;
		this.#chunk = undefined;
		this.#requestBody = false;
	}
}

export default HTTPClient;
