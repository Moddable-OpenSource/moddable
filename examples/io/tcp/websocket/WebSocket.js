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
 
 import Timer from "timer"
 
let urlRegExp = null;
let authorityRegExp = null;
function URLParts(url) {
	if (!urlRegExp)
		urlRegExp = new RegExp("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?");
	const urlParts = url.match(urlRegExp);
	if (!authorityRegExp)
		authorityRegExp = new RegExp("^([^:]+)(:(.*))?");
	const authorityParts = urlParts[4].match(authorityRegExp);
	return {
    	scheme:urlParts[2],
		host:authorityParts[1],
		port:authorityParts[3] ?? 80,
		path:urlParts[5],
    	query:urlParts[7],
		fragment:urlParts[9],
	}
}

const TypedArray = Object.getPrototypeOf(Int8Array);

class WebSocket {
	#buffers = [];
	#client = null;
	#eventListeners = {
		close:[],
		error:[],
		message:[],
		open:[],
	};
	#message = null;
	#protocol = "";
	#state = 0;
	#url = "";
	#writable = 0;
	#keepalive;
	
	constructor(url, protocol) {
		let options, keepalive;
		if (url instanceof Object) {
			options = url;
			url = options.url;
			protocol = options.protocol;
			keepalive = options.keepalive; 
		}
		if (url) {
			const parts = URLParts(url);
			if (parts.scheme !== "ws")
				throw new URIError("ws only");
			this.#url = url;
			if (protocol)
				this.#protocol = protocol;
			options = {
				...device.network.ws,
				host: parts.host,
				port: parts.port,
				path: parts.path,
				protocol: protocol
			}
		}
		this.#client = new device.network.ws.io({
			...options,
			onControl: (opcode, data) => {
				switch (opcode) {
					case this.#client.constructor.close: 
						this.#state = 3;
						data = new Uint8Array(data);
						const event = {
							code: (data[0] << 8) | data[1],
							reason: String.fromArrayBuffer(data.buffer.slice(2)),
							wasClean: true,
						};
						this.onclose(event);
						this.#eventListeners.close.forEach(listener => listener.call(null, event));
						break;

					case this.#client.constructor.ping:
						trace("PING!\n");
						break;

					case this.#client.constructor.pong:
						if (this.#keepalive)
							this.#keepalive.pong = true;
						break;
				}
			},
			onReadable: (count, options) => {
				trace(`onReadable ${count} binary ${options.binary} more ${options.more}\n`);
				if (!count)
					return;
				let data = this.#client.read(count);
				if (this.#state == 1) {
					let message = this.#message;
					if (message)
						message = message.concat(data);
					else
						message = data;
					if (options.more) 
						this.#message = message;
					else {
						this.#message = null;
						if (options.binary)
							data = message;
						else
							data = String.fromArrayBuffer(message);
						const event = { 
							data,
							// ??
						};
						this.onmessage(event);
						this.#eventListeners.message.forEach(listener => listener.call(null, event));
					}
				}
			},
			onWritable: (count) => {
// 				trace(`onWritable ${count}\n`);
				if (!count)
					return;
				this.#writable = count;
				if (this.#state == 0) {
					this.#state = 1;
					const event = {
						// ?? 
					};
					this.onopen(event);
    				this.#eventListeners.open.forEach(listener => listener.call(null, event));
					return;
				}
				
				let buffers = this.#buffers;
				while (buffers.length) {
					let buffer = buffers[0];
					let options = buffer.options;
					let data = buffer.data;
					const dataLength = data.byteLength;
					const writable = this.#writable;
					if (dataLength <= writable) {
						this.#writable = this.#client.write(data, options);
						buffers.shift();
					}
					else if (0 < writable) {
						let moreOptions = { ...options, more:true };
						this.#writable = this.#client.write(data.slice(0, writable), moreOptions);
						buffer.data = data.slice(writable);
						break;
					}
					else
						break;
				}
			},
			onClose: () => {
				trace(`onClose\n`);
			},
			onError: () => {
				trace(`onError\n`);
				this.#state = 3;
				const event = {
					// ?? 
				};
				this.onerror(event);
    			this.#eventListeners.error.forEach(listener => listener.call(null, event));
			}
		});
		
		if (keepalive) {
			this.#keepalive = Timer.repeat(timer => {
				if (!timer.pong) {
					this.#state = 3;
					const event = {
						message: "no pong response to keepalive" 
					};
					trace(event.message, "\n");
					this.onerror(event);
					this.#eventListeners.error.forEach(listener => listener.call(null, event));
					this.close();
					return;
				}

				if (this.#writable) {
					this.#writable = this.#client.write(new ArrayBuffer, {opcode: this.#client.constructor.ping});
					timer.pong = false;
				}
				else
					Timer.schedule(timer, 5000, keepalive);
			}, keepalive);
			this.#keepalive.pong = true;
		}
	}
	
	get binaryType() {
		return "arraybuffer";
	}
	set binaryType(it) {
		if (it != "arraybuffer")
			throw new Error("unsupported binaryType: " + it);
	}
	get bufferedAmount() {
		return this.#buffers.reduce((sum, buffer) => sum + buffer.data.byteLength, 0);
	}
	get extensions() {
		return ""; // ??
	}
	get protocol() {
		return this.#protocol;
	}
	get readyState() {
		return this.#state;
	}
	get url() {
		return this.#url;
	}
	addEventListener(event, listener) {
		let listeners = this.#eventListeners[event];
		if (!listeners)
			throw new Error("no such event");
		listeners.push(listener);
	}
	close(code, reason) {
		Timer.clear(this.#keepalive);
		this.#keepalive = undefined;

		if (code === undefined) {
			code = 1000;
			reason = "";
		}
		else {
			if ((code != 1000) && ((code < 3000) || (4999 < code)))
				throw new Error("invalid code: " + code);
			if (reason === undefined)
				throw new Error("code but no reason");
		}
		reason = ArrayBuffer.fromString(reason);
		if (reason.byteLength > 123)
			throw new Error("too long reason");
		if (this.#state == 1) {
			let data = new Uint8Array(2);
			data[0] = code >> 8;
			data[1] = code & 0xFF;
			data = data.buffer.concat(reason);
			this.#write(data, { opcode: this.#client.constructor.close });	
			this.#state = 2;
		}
	}
	onclose(event) {
	}
	onerror(event) {
	}
	onmessage(event) {
	}
	onopen(event) {
	}
	removeEventListener(event, listener) {
		let listeners = this.#eventListeners[event];
		if (!listeners)
			throw new Error("no such event");
		let index = listeners.find(item => item === listener);
		if (index >= 0)
			listeners.splice(index, 1);
	}
	send(data) {
		if (this.#state == 0)
			throw new Error("InvalidStateError");
		if (this.#state == 1) {
			let binary = false;
			if (data instanceof ArrayBuffer)
				binary = true;
			else if ((data instanceof DataView) || (data instanceof TypedArray)) {
				binary = true;
				data = data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength);
			}
			else
				data = ArrayBuffer.fromString(data);
			this.#write(data, { binary });	
		}
	}
	#write(data, options) {
		let buffers = this.#buffers;
		if (buffers.length)
			buffers.push({ data, options });
		else {
			const dataLength = data.byteLength;
			const writable = this.#writable;
			if (dataLength <= writable) {
				this.#writable = this.#client.write(data, options);
			}
			else if (0 < writable) {
				let moreOptions = { ...options, more:true };
				this.#writable = this.#client.write(data.slice(0, writable), moreOptions);
				buffers.push({ data: data.slice(writable), options });
			}
			else {
				buffers.push({ data, options });
			}
		}
	}
}

export default WebSocket;


