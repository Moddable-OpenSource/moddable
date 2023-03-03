/*
 * Copyright (c) 2021-2023  Moddable Tech, Inc.
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
import Base64 from "base64";
import Logical from "logical";

class WebSocketClient {
	#socket;
	#options;
	#state;
	#line;
	#data;
	#writable = 0;

	constructor(options) {
		this.#options = {
			onReadable: options.onReadable,
			onWritable: options.onWritable,
			onControl: options.onControl,
			onClose: options.onClose,
			onError: options.onError,
		};

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
		Timer.clear(this.#options.closer);
		Timer.clear(this.#options.timer);
		Timer.clear(this.#options.pending);
		delete this.#options.timer;
		delete this.#options.closer;
		delete this.#options.pending;
	}
	write(data, options) {
		const byteLength = data.byteLength;
		if (("connected" !== this.#state) || (byteLength > 65535) || ((byteLength + 8) > this.#writable))
			throw new Error;

		let type;
		if (options) {
			const opcode = options.opcode;
			if (undefined !== opcode)
				type = 0x80 | opcode;
			else {
				type = this.#options.fragmentedWrite ? 0 : (options.binary ? 2 : 1);
				if (options.more)
					this.#options.fragmentedWrite = true;
				else {
					type |= 0x80;
					delete this.#options.fragmentedWrite;
				}
			}
		}
		else {
			type = 0x81;
			delete this.#options.fragmentedWrite;
		}

		data = data.slice(0);
		const mask = Uint8Array.of(Math.random() * 256, Math.random() * 256, Math.random() * 256, Math.random() * 256);
		Logical.xor(data, mask.buffer);
		const format = this.#socket.format;
		this.#socket.format = "buffer";
		if (byteLength < 126) {
			this.#socket.write(Uint8Array.of(type, byteLength | 0x80, mask[0], mask[1], mask[2], mask[3]));
			this.#writable -= (6 + byteLength);
		}
		else {
			this.#socket.write(Uint8Array.of(type, 126 | 0x80, byteLength >> 8, byteLength, mask[0], mask[1], mask[2], mask[3]));
			this.#writable -= (8 + byteLength);
		}
		this.#socket.write(data);
		this.#socket.format = format;

		if (0x88 === type) {		// close
			if (this.#options.close & 2) {		// if we already received close, connection shuts down cleanly
				this.#options.closer = Timer.set(() => {	// can't invoke callback from write. wait. gives time for message to transmit too.
					this.close();
					this.#options.onClose?.call(this);
				}, 1000);
				this.#state = "closing";
			}
			else 
				this.#options.close = 1;		// set 1 to indicate that we've send close
		}

		return (this.#writable > 8) ? (this.#writable - 8) : 0;
	}
	read(count) {
		if (!this.#data)
			return;
		
		if ((undefined === count) || (count > this.#data))
			count = this.#data;

		const data = this.#socket.read(count);
		this.#data -= count;
		if (this.#options.mask) {
			const mask = this.#options.mask;
			Logical.xor(data, mask.buffer);
			if (this.#data) {
				switch (count & 3) {
					case 1:
						this.#options.mask = Uint8Array.of(mask[1], mask[2], mask[3], mask[0]);
						break;
					case 2:
						this.#options.mask = Uint8Array.of(mask[2], mask[3], mask[0], mask[1]);
						break;
					case 3:
						this.#options.mask = Uint8Array.of(mask[3], mask[0], mask[1], mask[2]);
						break;
				}
			}
		}
		
		if (!this.#data && this.#options.unread) {	// finished this message and have unread data pending on socket
			this.#options.timer = Timer.set(() => {
				delete this.#options.timer;
				this.#onReadable(this.#options.unread);
			});
		}

		return data;
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
						this.#socket.format = "buffer";

						if (this.#writable > 8)
							this.#options.onWritable?.call(this, this.#writable - 8);

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
						this.#socket.format = "number";
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
						try {
							this.#options.onControl?.call(this, opcode, control.buffer);
						}
						catch {
						}
						if (8 === opcode) {
							if (options.close & 1) {		// sent close, now receiving response: done
								this.close();
								this.#options.onClose?.call(this);
								return;
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
						this.#socket.format = "buffer";
						if (126 === options.length[0])
							options.length = (options.length[1] << 8) | options.length[2];
						else
							options.length = options.length[0];
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
					key[i] = (Math.random() * 256) | 0;
				
				const options = this.#options;
				let message = [
					`GET ${options.path || "/"} HTTP/1.1`, 
					`Host: ${options.host}`,
					`Upgrade: websocket`,
					`Connection: keep-alive, Upgrade`,
					`Sec-WebSocket-Version: 13`,
					`Sec-WebSocket-Key: ${Base64.encode(key.buffer)}`
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
				this.#socket.write(message); 
				this.#writable -= message.byteLength;

				this.#state = "receiveStatus"
				this.#line = "";
				options.flags = 0;
				this.#socket.format = "number";
				} break;
			
			case "connected":
				if (this.#options.pendingControl) {
					if ((this.#options.pendingControl.byteLength + 6) > this.#writable)
						return;
					
					Timer.clear(this.#options.pending);
					this.#options.pending = undefined;

					this.write(this.#options.pendingControl, {opcode: this.#options.pendingControl.opcode});
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
	
	static text = 1;
	static binary = 2;
	static close = 8;
	static ping = 9;
	static pong = 10;
}

export default WebSocketClient;
