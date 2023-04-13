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
 
 /*
	To do:

		use numbers for states instead of strings
*/

import Timer from "timer";

const Overhead = 8;
const BufferFormat = "buffer";
const NumberFormat = "number";

// const operationNames = [
// 	undefined,
// 	"CONNECT",
// 	"CONNACK",
// 	"PUBLISH",
// 	"PUBACK",
// 	"PUBREC",
// 	"PUBREL",
// 	"PUBCOMP",
// 	"SUBSCRIBE",
// 	"SUBACK",
// 	"UNSUBSCRIBE",
// 	"UNSUBACK",
// 	"PINGREQ",
// 	"PINGRESP",
// 	"DISCONNECT",
// ]
// function traceOperation(io, operation) {
// 	trace(`${io ? "<--" : "-->"} ${operationNames[operation]}\n`);
// }

class MQTTClient {
	#socket;
	#options;
	#state = "resolving";
	#writable = 0;		// to socket
	#readable = 0;		// from socket
	#payload = 0;		// from current message
	#parse = {state: 0};
	#id = 1;

	constructor(options) {
		this.#options = {
			host: options.host,
			port: options.port,
			id: options.id ?? "",
			clean: options.clean ?? true,
			keepalive: options.keepalive ?? 0,
			pending: []
		};

		if (!this.#options.host) throw new Error("host required");

		let value;
		if (value = options.onReadable) this.#options.onReadable = value; 
		if (value = options.onWritable) this.#options.onWritable = value; 
		if (value = options.onControl) this.#options.onControl = value; 
		if (value = options.onClose) this.#options.onClose = value; 
		if (value = options.onError) this.#options.onError = value; 
		if (value = options.user) this.#options.user = value; 
		if (value = options.password) this.#options.password = value; 
		if (value = options.will) this.#options.will = value; 
		
		const dns = new options.dns.io(options.dns);
		dns.resolve({
			host: this.#options.host, 

			onResolved: (host, address) => {
				try {
					this.#state = "connecting";
					this.#socket = new options.socket.io({
						...options.socket,
						address,
						host,
						port: this.#options.port ?? 1883,
						onReadable: count => this.#onReadable(count),
						onWritable: count => this.#onWritable(count),
						onError: error => this.#onError(error)
					});
					
					this.#options.connecting = Timer.set(() => {
						delete this.#options.connecting; 
						this.#onError();
					}, 30_000);		//@@ configurable
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
		Timer.clear(this.#options?.pending.timer);
		Timer.clear(this.#options?.timer);
		Timer.clear(this.#options?.keepalive);
		Timer.clear(this.#options?.connecting);
		this.#state = "closed";
		this.#socket?.close();
		this.#socket = undefined;
		this.#writable = 0;
		this.#readable = 0;
		this.#payload = 0;
		this.#options = undefined;
	}
	write(data, options) {
		const socket = this.#socket;
		socket.format = BufferFormat;

		if ("connected" !== this.#state) {
			const remaining = this.#options.remaining, byteLength = data.byteLength;
			if (("publishing" !== this.#state) || options || (byteLength > remaining))
				throw new Error;

			socket.write(data);
			this.#writable -= byteLength;

			this.#options.remaining -= byteLength;
			if (0 === this.#options.remaining) {
				delete this.#options.remaining;
				this.#state = "connected";
			}
				
			return (this.#writable > Overhead) ? (this.#writable - Overhead) : 0;
		}

		const id = options.id ?? ++this.#id;
		const operation = options.operation ?? MQTTClient.PUBLISH;
// 		traceOperation(true, operation);
		switch (operation) {
			case MQTTClient.PUBLISH: {
				let flags = 0, QoS = options.QoS ?? 0;
				let byteLength = options.byteLength ?? data.byteLength;
				if (data.byteLength > byteLength)
					throw new Error("invalid");

				const topic = makeStringBuffer(options.topic);
				const payload = topic.length + byteLength + (QoS ? 2 : 0);		// total payload size
				const length = 1 + getRemainingLength(payload) + topic.length + data.byteLength + (QoS ? 2 : 0);	// size of this fragment
				if (length > this.#writable)
					throw new Error("overflow");

				if (options.retain)
					flags |= 1; 
				if (QoS)
					flags |= (QoS & 3) << 1; 
				if (options.duplicate)
					flags |= 8;

				socket.write(Uint8Array.of((MQTTClient.PUBLISH << 4) | flags));
				writeRemainingLength(socket, payload);
				socket.write(topic);
				if (QoS)
					socket.write(Uint8Array.of(id >> 8, id));
				socket.write(data);

				if (data.byteLength < byteLength) {
					this.#state = "publishing";
					this.#options.remaining = byteLength - data.byteLength;  
				}

				this.#writable -= length;
				} break;

			case MQTTClient.SUBSCRIBE: {
				let items = options.items, count = items.length, topics = [];
				let payload = count + 2;
				for (let i = 0; i < count; i++) {
					const topic = makeStringBuffer(items[i].topic);
					payload += topic.length;
					topics.push(topic);
				}
				const length = 1 + getRemainingLength(payload) + payload;
				if (length > this.#writable)
					throw new Error("overflow");

				socket.write(Uint8Array.of((MQTTClient.SUBSCRIBE << 4) | 2));
				writeRemainingLength(socket, payload);
				socket.write(Uint8Array.of(id >> 8, id));
				for (let i = 0; i < count; i++) {
					const QoS = items[i].QoS ?? 0;
					socket.write(topics[i]);
					socket.write(Uint8Array.of(QoS & 3));
				}
				this.#writable -= length;
				} break;

			case MQTTClient.UNSUBSCRIBE: {
				let items = options.items, count = items.length, topics = [];
				let payload = 2;
				for (let i = 0; i < count; i++) {
					const topic = makeStringBuffer(items[i].topic);
					payload += topic.length;
					topics.push(topic);
				}
				const length = 1 + getRemainingLength(payload) + payload;
				if (length > this.#writable)
					throw new Error("overflow");

				socket.write(Uint8Array.of((MQTTClient.UNSUBSCRIBE << 4) | 2));
				writeRemainingLength(socket, payload);
				socket.write(Uint8Array.of(id >> 8, id));
				for (let i = 0; i < count; i++)
					socket.write(topics[i]);

				this.#writable -= length;
				} break;

			case MQTTClient.PUBREL:
			case MQTTClient.PUBACK:
			case MQTTClient.PUBCOMP:
			case MQTTClient.PUBREC: {
				const payload = 2;
				const length = 1 + getRemainingLength(payload) + payload;
				if (length > this.#writable)
					throw new Error("overflow");

				socket.write(Uint8Array.of((operation << 4) | ((MQTTClient.PUBREL === operation) ? 2 : 0)));
				writeRemainingLength(socket, payload);
				socket.write(Uint8Array.of(id >> 8, id));

				this.#writable -= length;
				} break;

			case MQTTClient.DISCONNECT:
			case MQTTClient.PINGREQ:
				if (2 > this.#writable)
					throw new Error("overflow");

				socket.write(Uint8Array.of(operation << 4, 0));
				this.#writable -= 2;
				break;

			default:
				throw new Error("unknown");
		}
		
		return (this.#writable > Overhead) ? (this.#writable - Overhead) : 0;
	}
	read(count) {
		if (count > this.#payload) {
			count = this.#payload;
			if (!count)
				return;
		}

		this.#readable -= count;
		this.#payload -= count;
		this.#socket.format = BufferFormat;
		const result = this.#socket.read(count);

		if ((0 === this.#payload) && this.#parse) {	// full message read and not currently running the parser
			this.#options.timer ??= Timer.set(() => {
				delete this.#options.timer;
				if (this.#readable > 0)
					this.#onReadable(this.#readable);		// this results in this.#options.last being advanced inaccurately
			});
		}

		return result;
	}
	#onReadable(count) {
		this.#readable = count;
		this.#options.last = Date.now();

		if (this.#payload)
			return;

		const socket = this.#socket;
		socket.format = NumberFormat;

		let parse = this.#parse, byte;
		this.#parse = undefined;		// signal that parser is running
	parser:
		while (count--) {			//@@ verify that each step consumes exactly one byte (or updates count!)
			switch (parse.state) {
				case 0:
					byte = socket.read();
					parse.flags = byte & 0x0F;
					parse.operation = byte >> 4;
					if ((0 === parse.operation) || (15 === parse.operation)) {
						this.#onError("invalid operation");
						break parser;
					}

					parse.state = 1;
					break;

				// remaining length
				case 1:
					byte = socket.read();
					parse.length = byte & 0x7F;
					if (byte & 0x80)
						parse.state = 2
					else {
						parse.state = parse.operation << 4;
						if (!parse.length) {		// no payload - handle immediately (PINGREQ, PINGRESP, DISCONNECT)
							if (this.#parsed(parse))
								return;
							parse = {state: 0};
						} 
					}
					break;

				case 2:
					byte = socket.read();
					parse.state = byte & 0x80 ? 3 : (parse.operation << 4);
					parse.length += (byte & 0x7F) << 7;
					break;

				case 3:
					byte = socket.read();
					parse.state = byte & 0x80 ? 4 : (parse.operation << 4);
					parse.length += (byte & 0x7F) << 14;
					break;

				case 4:
					byte = socket.read();
					if (byte & 0x80) {
						this.#onError("bad data");
						break parser;
					}
					parse.length += (byte & 0x7F) << 21;
					parse.state = (parse.operation << 4);
					break;
/*
				// CONNECT
				case 0x10:
					if (!parse.expect)
						parse.expect = Array.of(0, 4, 77, 81, 84, 84, 4);
					if (socket.read() !== parse.expect.shift()) {
						this.#onError("bad connect");
						break parser;
					}
					if (!parse.expect.length) {
						delete parse.expect;
						parse.state = 0x11;
					}
					break;

				case 0x11:
					parse.flags = socket.read();
					if (parse.flags & 1) {
						this.#onError("reserved not zero");
						break parser;
					}
					parse.state = 0x12;
					break;

				case 0x12:
					parse.connect = {keepalive: socket.read() << 8};
					parse.state = 0x13;
					break;

				case 0x13:
					parse.connect.keepalive |= socket.read();
					parse.state = 0x14;
					parse.length -= 10;
					parse.buffer = new Uint8Array(parse.length);
					parse.buffer.position = 0;
					parse.state = 0x14;
					break;

				case 0x14:
					parse.buffer[parse.buffer.position++] = socket.read();
					if (parse.buffer.position === parse.length) {
						const connect = parse.connect;
						parse.buffer.position = 0;
						connect.id = String.fromArrayBuffer(getBuffer(parse.buffer));
						if (parse.flags & 4) {	// will
							connect.will = {topic: String.fromArrayBuffer(getBuffer(parse.buffer))};
							connect.will.message = getBuffer(parse.buffer);
						}
						if (parse.flags & 0x80)
							connect.user = String.fromArrayBuffer(getBuffer(parse.buffer));
						if (parse.flags & 0x40)
							connect.password = getBuffer(parse.buffer);
						if (this.#parsed(parse))
							return;
						this.ws.write(Uint8Array.of(0x20, 2, 1, result).buffer);		// CONNACK
						parse = {state: 0};
					}
					break;
*/
				// PUBLISH
				case 0x30:
					parse.topic = socket.read() << 8;
					parse.state = 0x31;
					break;

				case 0x31:
					parse.topic |= socket.read();
					parse.length -= parse.topic + 2;
					if (0 === parse.topic) {
						this.#onError("empty topic");
						break parser;
					}
					parse.topic = new Uint8Array(parse.topic);
					parse.topic.position = 0;
					parse.state = 0x32;
					break;

				case 0x32:		// optimization: consume as many bytes as possible (see state 0x36)
					parse.topic[parse.topic.position++] = socket.read();
					if (parse.topic.position === parse.topic.length) {
						parse.topic = String.fromArrayBuffer(parse.topic.buffer);
						parse.QoS = (parse.flags >> 1) & 3;
						parse.retain = (parse.flags & 1) !== 0;
						parse.state = ((1 == parse.QoS) || (2 === parse.QoS)) ? 0x34 : 0x36;
						parse.remaining = parse.length;
					}
					break;

				case 0x34:
					parse.id = socket.read() << 8; 
					parse.length -= 1;
					parse.state += 1;
					break;

				case 0x35:
					parse.id |= socket.read(); 
					parse.length -= 1;
					parse.state += 1;
					parse.remaining -= 2;
					break;

				case 0x36:
					if (parse.remaining) {
						const options = {};
						if (parse.topic) {
							options.topic = parse.topic;
							delete parse.topic;
							options.byteLength = parse.remaining;
							options.QoS = parse.QoS;
						}

						count += 1;
						const available = (parse.remaining < count) ? parse.remaining : count; 
						this.#payload = available;
						parse.remaining -= available; 
//						count -= available;
						this.#readable = count;
						options.more = parse.remaining !== 0;
						if (!options.more) {
							if (parse.QoS) {
								this.#queue({
									operation: (1 === parse.QoS) ? MQTTClient.PUBACK : MQTTClient.PUBREC,
									id: parse.id,
									byteLength: 4
								});
							}
							parse = {state: 0};
						}
// 						traceOperation(false, MQTTClient.PUBLISH);
						this.#options.onReadable?.call(this, available, options);
						socket.format = NumberFormat;
						count = this.#readable;
					}
					if (this.#payload)
						break parser;		// unread data
					break;

				// CONNACK
				case 0x20:
					if (2 !== parse.length) {
						this.#onError("unexpected length");
						break parser;
					}
					socket.read();	// connect acknowledge flags
					parse.length -= 1;
					parse.state = 0x21;
					break;

				case 0x21:
					parse.returnCode = socket.read();
					if (this.#parsed(parse))
						return;
					parse = {state: 0};
					break;

/*
				case 0x80:		// SUBSCRIBE
				case 0xA0:		// UNSUBSCRIBE
					parse.id = socket.read() << 8;					// MSB packet ID
					parse.state = 0x81;
					break;

				case 0x81:
					parse.id |= socket.read();						// LSB packet ID
					parse.length -= 2;
					parse.buffer = new Uint8Array(parse.length);	// capture payload bytes
					parse.buffer.position = 0;
					parse.state = 0x82;
					break;

				case 0x82:
					parse.buffer[parse.buffer.position++] = socket.read();
					if (parse.buffer.position === parse.length) {
						const result = (MQTTClient.SUBSCRIBE === parse.operation) ? [] : null;
						parse.buffer.position = 0;
						while (parse.buffer.position < parse.buffer.length) {
							parse.topic = String.fromArrayBuffer(getBuffer(parse.buffer));
							if (result) {
								parse.qos = parse.buffer[parse.buffer.position++];
								result.push(this.#parsed(parse) ?? 0);
							}
							else
								this.#parsed(parse)
						}
						if (result) {
							this.ws.write(Uint8Array.of(0x90, result.length + 2, parse.id >> 8, parse.id & 0xFF).buffer);	// SUBACK
							this.ws.write(Uint8Array.from(result).buffer);
						}
						else
							this.ws.write(Uint8Array.of(0xB0, 2, parse.id >> 8, parse.id & 0xFF).buffer);					// UNSUBACK
						parse = {state: 0};
					}
					break;
*/

				// PUBACK, PUBREC, PUBREL, PUBCOMP, SUBACK, UNSUBACK
				case 0x40:
				case 0x50:
				case 0x60:
				case 0x70:
				case 0x90:
				case 0xB0:		// MSB packet ID
					parse.id = socket.read() << 8;
					parse.length -= 1;
					if ((MQTTClient.SUBACK << 4) === parse.state)
						parse.payload = [];
					parse.state = 0x91;
					break;

				case 0x91:		// LSB packet ID
					parse.id |= socket.read();
					parse.length -= 1;
					parse.state = 0x92;
					if (parse.length)
						break;			// more bytes

				case 0x92:		// ignore remaining bytes
					if (parse.length) {
						byte = socket.read();
						parse.payload?.push(byte);
						parse.length -= 1;
					}
					if (0 === parse.length) {
						if (this.#parsed(parse))
							return;
						parse = {state: 0};
					}
					break;

				default:
					this.#onError("bad parse state");
					break parser;
			}
		}

		this.#readable = count;
		this.#parse = parse;
	}
	#onWritable(count) {
		this.#writable = count;

		const options = this.#options;
		const socket = this.#socket;
		const state = this.#state;
		if ("connecting" === state) {
			Timer.clear(options.connecting);
			delete options.connecting;

			const keepalive = Math.round(options.keepalive / 1000);
			const id = makeStringBuffer(options.id);
			const user = makeStringBuffer(options.user);
			const password = makeStringBuffer(options.password);
			const will = options.will;
			const topic = makeStringBuffer(will?.topic);
			const message = makeStringBuffer(will?.message);

			const flags =	(options.clean ? 2 : 0) |		// CleanSession
							(user.length ? 0x80 : 0) | 
							(password.length ? 0x40 : 0) |
							((topic.length && message.length) ? 0x04 : 0) |
							(will?.retain ? 0x20 : 0) | 
							(((will?.QoS ?? 0) & 3) << 3);

			const payload = 10 + id.length + topic.length + message.length + user.length + password.length;
			const length = 1 + getRemainingLength(payload) + payload;

// 			traceOperation(true, MQTTClient.CONNECT);
			socket.write(Uint8Array.of(MQTTClient.CONNECT << 4));
			writeRemainingLength(socket, payload);

			socket.write(Uint8Array.of(
				0x00, 0x04,
				77, 81, 84, 84,							// protocol name MQTT
				0x04,									// protocol level 4 (MQTT version 3.1.1)
				flags,
				keepalive >> 8, keepalive				// keepalive in seconds
			));

			if (id.length) socket.write(id);
			if (topic.length) socket.write(topic);
			if (message.length) socket.write(message);
			if (user.length) socket.write(user);
			if (password.length) socket.write(password);

			this.#writable -= length;

			delete options.host;
			delete options.address;
			delete options.port;
			delete options.id;
			delete options.user;
			delete options.password;
			delete options.clean;
			delete options.will;

			if (keepalive) {
				options.keepalive = Timer.repeat(() => this.#keepalive(), keepalive * 500);
				options.keepalive.interval = keepalive * 1000;
				options.last = Date.now();
			}

			this.#state = "login";
			return;
		}

		if (("connected" === state) || ("publishing" === state)) {
			if ("connected" === state) {
				const pending = this.#options.pending;
				while (pending.length) {
					if (this.#writable < pending[0].byteLength)
						return;
					this.write(null, pending.shift());
				}
			}
			if (this.#writable > Overhead)
				this.#options.onWritable?.call(this, this.#writable - Overhead);
		}
	}
	#onError(msg) {
		trace("mqttClient error: ", msg ?? "unknown", "\n");
		this.#options.onError?.call(this);
		this.close();
	} 
	#parsed(msg) {
		const operation = msg.operation;
// 		traceOperation(false, operation);
		if (MQTTClient.CONNACK === operation) {
			if (msg.returnCode)
				return void this.#onError("connection rejected")
			this.#state = "connected";
		}
		else if ((MQTTClient.PUBREC === operation) || (MQTTClient.PUBREL === operation)) {
			this.#queue({
				operation: (MQTTClient.PUBREC === operation) ? MQTTClient.PUBREL : MQTTClient.PUBCOMP,
				id: msg.id,
				byteLength: 4
			});
		}

		const onControl = this.#options.onControl;
		if (onControl) {
			delete msg.length;
			delete msg.flags;
			delete msg.state;
			delete msg.remaining;
			onControl.call(this, msg);
		}

		if ((MQTTClient.CONNACK === operation) && (this.#writable > Overhead))
			this.#options.onWritable?.call(this, this.#writable - Overhead);

		if (this.#socket) {
			this.#socket.format = NumberFormat;
			return;
		}

		return true;
	}
	#keepalive() {
		const options = this.#options;
		const interval = options.keepalive.interval;
		const now = Date.now();
		if ((options.last + (interval >> 1)) > now)
			return;		// received data within the keepalive interval

		if ((options.last + (interval + (interval >> 1))) < now)
			return void this.#onError("time out"); // no response in too long

		for (let i = 0, queue = this.#options.pending, length = queue.length; i < length; i++) {
			if (queue[i].keepalive && (MQTTClient.PINGREQ === queue[i].operation))
				return void this.#onError("time out"); // unsent keepalive ping, exit
		}

		this.#queue({
			operation: MQTTClient.PINGREQ,
			byteLength: 2,
			keepalive: true
		});
	}
	#queue(message) {
		const pending = this.#options.pending;
		pending.push(message);
		pending.timer ??= Timer.set(() => {
			delete this.#options.pending.timer;
			this.#onWritable(this.#writable);
		});
	}

	static CONNECT = 1;
	static CONNACK = 2;
	static PUBLISH = 3;
	static PUBACK = 4;
	static PUBREC = 5;
	static PUBREL = 6;
	static PUBCOMP = 7;
	static SUBSCRIBE = 8;
	static SUBACK = 9;
	static UNSUBSCRIBE = 10;
	static UNSUBACK = 11;
	static PINGREQ = 12;
	static PINGRESP = 13;
	static DISCONNECT = 14;
}

function makeStringBuffer(string) {
	if (undefined === string)
		return new Uint8Array(0);

	if (!(string instanceof ArrayBuffer))
		string = ArrayBuffer.fromString(string);
	string = new Uint8Array(string);
	const result = new Uint8Array(string.length + 2);
	result[0] = string.length >> 8;
	result[1] = string.length;
	result.set(string, 2)

	return result;
}

function getRemainingLength(length) {
	if (length < 128)
		return 1;
	else if (length < 16384)
		return 2;
	else if (length < 2097152)
		return 3;
	return 4;
}

function writeRemainingLength(socket, length) {
	if (length < 128)
		socket.write(Uint8Array.of(length));
	else if (length < 16384)
		socket.write(Uint8Array.of(0x80 | (length & 0x7F), length >> 7));
	else if (length < 2097152)
		socket.write(Uint8Array.of(0x80 | (length & 0x7F), 0x80 | ((length >> 7) & 0x7F), length >> 14));
	else
		socket.write(Uint8Array.of(0x80 | (length & 0x7F), 0x80 | ((length >> 7) & 0x7F), 0x80 | ((length >> 14) & 0x7F), length >> 21));
}

function getBuffer(buffer) {
	const position = buffer.position;
	const length = (buffer[position] << 8) | buffer[position + 1];
	const result = buffer.buffer.slice(position + 2, position + 2 + length);
	buffer.position += 2 + length;
	return result;
}

export default MQTTClient;
