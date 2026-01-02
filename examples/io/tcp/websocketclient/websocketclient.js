/*
 * Copyright (c) 2021-2025  Moddable Tech, Inc.
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
	To do:
	
		- minimize setting of #socket.format
*/

import Timer from "timer";
import Logical from "logical";

const BufferFormat = "buffer";
const NumberFormat = "number";

class WebSocketClient {
	#socket;
	#options;
	#state;
	#line;
	#data;
	#format;		// true for number, false for buffer
	#writable = 0;
	#mask;			// write mask

	constructor(options) {
		this.#options = {
			onReadable: options.onReadable,
			onWritable: options.onWritable,
			onControl: options.onControl,
			onClose: options.onClose,
			onError: options.onError,
		};

		this.format = options.format ?? BufferFormat;
		const target = options.target;
		if (undefined !== target)
			this.target = target;

		const attach = options.attach;
		if (attach) {
			this.#socket = new attach.constructor({
				from: attach,
				onReadable: count => this.#onReadable(count),
				onWritable: count => this.#onWritable(count),
				onError: () => this.#onError()
			});
			this.#state = "connected";
			return;
		}

		this.#mask = new Uint8Array(4);		// client, so mask (attach case is server, so no mask)

		this.#options.host = options.host; 
		this.#options.path = options.path; 
		this.#options.port = options.port ?? 80; 
		this.#options.protocol = options.protocol; 
		this.#options.headers = options.headers; 

		const dns = new options.dns.io(options.dns);
		this.#state = "resolving";
		dns.resolve({
			host: this.#options.host, 

			onResolved: (host, address) => {
				try {
					this.#state = "connecting";
					this.#socket = new options.socket.io({
						...options.socket,
						address,
						host,
						port: this.#options.port,
						onReadable: count => this.#onReadable(count),
						onWritable: count => this.#onWritable(count),
						onError: () => this.#onError()
					});
				}
				catch {
					this.#onError?.();
				}
			},
			onError: () => {
				this.#onError?.();
			},
		});
	}
	close() {
		this.#state = "closed";
		this.#socket?.close();
		this.#socket = undefined;
		const options = this.#options;
		Timer.clear(options.closer);
		Timer.clear(options.timer);
		Timer.clear(options.pending);
		delete options.timer;
		delete options.closer;
		delete options.pending;
	}
	write(data, options) {
		const mask = this.#mask;
		const byteLength = data.byteLength;
		const header = ((byteLength < 126) ? 2 : ((byteLength < 65536) ? 4 : 10)) + (mask ? 4 : 0);
		const total = byteLength + header;
		if (("connected" !== this.#state) || (total > this.#writable))
			throw new Error;

		let type;
		if (options) {
			const opcode = options.opcode;
			if (undefined !== opcode)
				type = 0x80 | opcode;
			else {
				type = this.#options.fragmentedWrite ? 0 : ((options.binary ?? true) ? 2 : 1);
				if (options.more)
					this.#options.fragmentedWrite = true;
				else {
					type |= 0x80;
					delete this.#options.fragmentedWrite;
				}
			}
		}
		else {
			if (this.#options.fragmentedWrite) {
				type = 0x80;
				delete this.#options.fragmentedWrite;
			}
			else
				type = 0x82;
		}

		if (ArrayBuffer.isView(data)) {
			if (data.BYTES_PER_ELEMENT > 1)
				throw new TypeError;		// not a Byte Buffer
		}
		else
			data = new Uint8Array(data);
		const payload = new Uint8Array(total);
		payload[0] = type;
		if (byteLength < 126)
			payload[1] = byteLength;
		else if (byteLength < 65536) {
			payload[1] = 126;
			payload[2] = byteLength >> 8;
			payload[3] = byteLength;
		}
		else {
			payload[1] = 127;
			payload[2] = 0;
			payload[3] = byteLength >> 48;
			payload[4] = byteLength >> 40;
			payload[5] = byteLength >> 32;
			payload[6] = byteLength >> 24;
			payload[7] = byteLength >> 16;
			payload[8] = byteLength >> 8;
			payload[9] = byteLength;
		}

		payload.set(data, header);		//@@ incorrect if data is Int8Array or DataView... native function to copy with optional mask?
		if (mask) {
			payload[1] |= 0x80;

			mask[0] = Math.irandom(256);
			mask[1] = Math.irandom(256);
			mask[2] = Math.irandom(256);
			mask[3] = Math.irandom(256);
			Logical.xor(payload.subarray(header), mask.buffer);
			payload.set(mask, header - 4);
		}

		this.#socket.format = BufferFormat;
		this.#writable = this.#socket.write(payload);

		if (0x88 === type) {		// close
			if (this.#options.close & 2) {		// if we already received close, connection shuts down cleanly
				this.#options.closer = Timer.set(() => {	// can't invoke callback from write. wait. gives time for message to transmit too.
					this.close();
					this.#options.onClose?.call(this);
				}, 1000);
				this.#state = "closing";
			}
			else 
				this.#options.close = 1;		// set 1 to indicate that we've sent close
		}

		const writable = this.#writable - (2 + 8 + (mask ? 4 : 0));
		return (writable > 0) ? writable : 0;
	}
	read(count) {
		if (!this.#data)
			return;

		if (this.#format) {
			this.#data--;
			this.#socket.format = NumberFormat;
			return this.#socket.read();
		}

		this.#socket.format = BufferFormat;

		if (undefined === count)
			count = this.#data;

		let data, result;

		if (NumberFormat === typeof count) {
			if (count > this.#data)
				count = this.#data;

			data = result = this.#socket.read(count);
		}
		else {
			data = count;
			count = result = this.#socket.read(data);
		}
		this.#data -= count;

		const options = this.#options;
		if (options.mask) {
			const mask = options.mask;
			Logical.xor(data, mask, count);
			if (this.#data) {
				switch (count & 3) {
					case 1:
						options.mask = Uint8Array.of(mask[1], mask[2], mask[3], mask[0]);
						break;
					case 2:
						options.mask = Uint8Array.of(mask[2], mask[3], mask[0], mask[1]);
						break;
					case 3:
						options.mask = Uint8Array.of(mask[3], mask[0], mask[1], mask[2]);
						break;
				}
			}
		}

		if (!this.#data && options.unread) {	// finished this message and have unread data pending on socket
			options.timer = Timer.set(() => {
				delete this.#options.timer;
				this.#onReadable(this.#options.unread);
			});
		}

		return result;
	}
	#onReadable(count) {
		switch (this.#state) {
			case "receiveStatus":
			case "receiveHeader":
				do {
					while (count--) {
						const c = this.#socket.read();
						this.#line += String.fromCharCode(c);
						if (10 === c)
							break;
					}

					if (!this.#line.endsWith("\r\n"))
						return;

					if ("receiveStatus" === this.#state) {
						let status = this.#line.split(" ");
						if (status.length < 3)
							return void this.#onError();
						status = parseInt(status[1]);
						if (101 !== status)
							return void this.#onError();
						this.#state = "receiveHeader";
					}
					else if ("\r\n" === this.#line) {
						// done
						if (7 !== this.#options.flags)
							return void this.#onError();
						this.#state = "connected";
						delete this.#options.flags;
						delete this.#options.host;
						delete this.#options.path;
						delete this.#options.port;
						this.#socket.format = BufferFormat;

						if (this.#writable > 8) {
							this.#options.onWritable?.call(this, this.#writable - 8);
							if (!this.#socket)
								return;
						}

						if (count)
							return void this.#onReadable(count);	// more data to read - run "connected"
						break;
					}
					else {
						const position = this.#line.indexOf(":");
						const name = this.#line.substring(0, position).trim().toLowerCase();
						const data = this.#line.substring(position + 1).trim();

						if (("connection" === name) && ("upgrade" === data.toLowerCase()))
							this.#options.flags |= 1;
						else if ("sec-websocket-accept" === name)
							this.#options.flags |= 2;		//@@ validate data
						else if (("upgrade" == name) && ("websocket" == data.toLowerCase()))
							this.#options.flags |= 4;
					}

					this.#line = "";
				} while (true);
				break;

			case "connected": {
				const options = this.#options;

				Timer.clear(options.timer);
				delete options.timer;

				if (this.#data)
					return;

				while (count) {
					if (undefined === options.tag) {
						this.#socket.format = NumberFormat;
						let tag = options.tag = this.#socket.read();
						count--;

						if (tag & 0x70)
							return void this.#onError();

						tag &= 0x0F;
						if (1 === tag & 0x0F)
							options.binary = false;
						else if (2 === tag)
							options.binary = true;
						else if (8 & tag)
							options.control = true;
						else if (tag)
							return void this.#onError();
						continue;
					}
					if (undefined === options.length) {
						let length = this.#socket.read();
						count--;
						if (length & 0x80) {
							length &= 0x7F;
							options.mask = [];
						}
						else
							delete options.mask;
						options.length = [length];
						if (length)
							continue;
						// 0 length payload. process immediately
					}
					if ((126 === options.length[0]) && (options.length.length < 3)) {
						options.length.push(this.#socket.read());
						count--;
						continue;
					}
					if ((127 === options.length[0]) && (options.length.length < 9)) {
						options.length.push(this.#socket.read());
						count--;
						continue;
					}
					if (options.mask && options.mask.length < 4) {
						//@@ it is an error for client to receieve a mask. this code applies to future server. client should fail here.
						options.mask.push(this.#socket.read());
						count--;
						if (4 !== options.mask.length)
							continue;

						options.mask = Uint8Array.from(options.mask);
						if (options.length[0])
							continue;
						// 0 length payload. process immediately
					}

					if (options.control) {
						if (true === options.control) {
							options.length = options.length[0];		//@@ assert 1 === options.length
							options.control = new Uint8Array(options.length);
							options.control.position = 0;
						}
						const control = options.control;
						while (count && (control.position < options.length)) {
							control[control.position++] = this.#socket.read();
							count--;
						}
						if (control.position !== options.length)
							return;

						const opcode = options.tag & 0x0F;
						if (options.mask) {
							Logical.xor(control.buffer, options.mask.buffer);
						}
						try {
							this.#options.onControl?.call(this, opcode, control.buffer);
							if (!this.#socket)
								return;
						}
						catch {
							/* this space intentionally left blank */
						}
						if (8 === opcode) {
							if (options.close & 1) {		// sent close, now receiving response: done
								this.close();
								return void this.#options.onClose?.call(this);
							}
							else {						
								options.close = 2;			// received request for clean close: reply

								options.pendingControl = control.buffer;
								options.pendingControl.opcode = 8;

								this.#options.pending ??= Timer.set(() => {
									delete this.#options.pending;
									this.#onWritable(this.#writable);
								});
							}
						}
						else if (9 === opcode) {	// ping
							if (!options.pendingControl && !(2 & options.close)) {
								options.pendingControl = control.buffer;
								options.pendingControl.opcode = 10;

								this.#options.pending = Timer.set(() => {
									delete this.#options.pending;
									this.#onWritable(this.#writable);
								});
							}
						}
						else if (10 === opcode)	// pong
							;
						else
							return void this.#onError();

						delete options.tag;
						delete options.control;
						delete options.length;
						continue;
					}

					if (!options.ready) {
						options.ready = true;
						this.#socket.format = BufferFormat;
						let length = options.length[0];
						if (127 === length) {
							let i = 1;
							while (i < 9) {
								length = options.length[i++];
								if (length)
									break;
							}
							while (i < 9)
								length = (length << 8) | options.length[i++];
						}
						else if (126 === length)
							length = (options.length[1] << 8) | options.length[2];
// 						trace(`LENGTH ${ options.length[0] } ${ length }\n`);
						options.length = length;
					}

					let read, more, binary = options.binary;
					if (options.length <= count) {
						more = !(options.tag & 0x80);
						this.#data = read = options.length;
						delete options.ready;
						delete options.length;
						if (!more)
							delete options.binary;
						delete options.tag;
					}
					else {
						more = true;
						this.#data = read = count;
						options.length -= count;
					}
					delete options.unread;
					options.onReadable?.call(this, this.#data, {more, binary});
					if (!this.#socket)
						break;
					
					count -= (read - this.#data);
					if (this.#data) {
						options.unread = count;
						break;
					}
				}
				} break;

			default:
//				trace(`ignoring onReadable in state ${this.#state}\n`);
				break;
		}
	}
	#onWritable(count) {
		this.#writable = count;

		switch (this.#state) {
			case "connecting": {
				const key = new Uint8Array(16);
				for (let i = 0; i < 16; i++)
					key[i] = Math.irandom(256);
				
				const options = this.#options;
				let message = [
					`GET ${options.path || "/"} HTTP/1.1`, 
					`Host: ${options.host}`,
					`Upgrade: websocket`,
					`Connection: keep-alive, Upgrade`,
					`Sec-WebSocket-Version: 13`,
					`Sec-WebSocket-Key: ${key.toBase64()}`
				];
				if (options.protocol)
					message.push(`Sec-WebSocket-Protocol: ${options.protocol}`);

				if (options.headers) {
					for (const [header, value] of options.headers)
						message.push(`${header}: ${value}`);
				}

				delete options.protocol;
				delete options.headers;

				message.push("", "");

				//@@ if headers exceed count, send in pieces
				message = ArrayBuffer.fromString(message.join("\r\n"));
				this.#writable = this.#socket.write(message); 

				this.#state = "receiveStatus"
				this.#line = "";
				options.flags = 0;
				this.#socket.format = NumberFormat;
				} break;
			
			case "connected":
				if (this.#options.pendingControl) {
					if ((this.#options.pendingControl.byteLength + 6) > this.#writable)
						return;
					
					Timer.clear(this.#options.pending);
					this.#options.pending = undefined;

					this.#writable = this.write(this.#options.pendingControl, {opcode: this.#options.pendingControl.opcode});
					delete this.#options.pendingControl;
				}
				this.#options.onWritable?.call(this, (this.#writable <= 8) ? 0 : (this.#writable - 8));
				break;

			default:
//				trace(`ignoring onWritable in state ${this.#state}\n`);
				break;
		}
	}
	#onError() {
		this.close();
		if (this.#options.close)
			this.#options.onClose?.call(this);
		else
			this.#options.onError?.call(this);
	}
	get format() {
		return this.#format ? NumberFormat : BufferFormat;
	}
	set format(value) {
		if (BufferFormat === value)
			this.#format = false;
		else if (NumberFormat === value)
			this.#format = true;
		else
			throw new RangeError;
	}
	
	static text = 1;
	static binary = 2;
	static close = 8;
	static ping = 9;
	static pong = 10;
}

export default WebSocketClient;