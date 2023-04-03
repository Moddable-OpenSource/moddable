/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

import {Client as WSClient} from "websocket";
import {Socket} from "socket";
import Timer from "timer";

/*
 * Implements a basic MQTT client. Upon creation of a client instance, provides methods for
 * subscribing and unsubscribing to topics, publishing messages to topics, and pinging if you really
 * want to. Currently provides callbacks only for connection-ready, messages received on subscribed
 * channels, and closing.
 *
 * Supports MQTT over websockets and direct socket connections
 *
 *     import {Client} from "mqtt";
 *     let client = new Client({host: hostname, port: port, path: "/", id: "myclient"});
 *     client.onReady = () => { trace('connection up\n'); client.subscribe("/foo"); };
 *     client.onMessage = (t, b) => { trace(`received message on ${t} with body ${String.fromArrayBuffer(b)}\n`); };
 *     client.onClose = () => { trace(`server closed connection\n`); };
 *     client.publish("/foo", "bar");
 *
 * This implementation is QoS 0 only.
 */

const CONNECT = 0x10;
const CONNACK = 0x20;
const PUBLISH = 0x30;
const SUBSCRIBE = 0x80;
const UNSUBSCRIBE = 0xA0;

export default class Client {
	#timer;
	#messages;

	constructor(dictionary) {
		this.state = 0;

		if (dictionary.socket) {
			this.server = true;
			this.ws = dictionary.socket;
			this.ws.callback = socket_callback.bind(this);
			return;
		}

		if (!dictionary.host)
			throw new Error("parameter host is required");

		this.connect = {id: dictionary.id ?? ""};
		if (dictionary.user)
			this.connect.user = dictionary.user;
		if (dictionary.password)
			this.connect.password = dictionary.password;
		if (dictionary.will)
			this.connect.will = dictionary.will;

//		// set default callbacks to be overridden by caller
//		this.onConnected = function() {};
//		this.onReady = function() {};
//		this.onMessage = function() {};
//		this.onClose = function() {};

		const path = dictionary.path ?? null; // includes query string
		this.timeout = dictionary.timeout ?? 0;

		if (path) {
			// presence of this.path triggers WebSockets mode, as MQTT has no native concept of path
			const port = dictionary.port ? dictionary.port : 80;
			if (dictionary.Socket)
				this.ws = new WSClient({host: dictionary.host, port, path, protocol: "mqtt", Socket: dictionary.Socket, secure: dictionary.secure});
			else
				this.ws = new WSClient({host: dictionary.host, port, path, protocol: "mqtt"});
			this.ws.callback = ws_callback.bind(this);
		} else {
			const port = dictionary.port ? dictionary.port : 1883;
			if (dictionary.Socket)
				this.ws = new (dictionary.Socket)({host: dictionary.host, port, secure: dictionary.secure});
			else
				this.ws = new Socket({host: dictionary.host, port});
			this.ws.callback = socket_callback.bind(this);
		}
	}
	publish(topic, data, flags) {
		if (this.state < 2)
			throw new Error("connection closed");

		++this.packet;

		topic = makeStringBuffer(topic);
		if (!(data instanceof ArrayBuffer))
			data = ArrayBuffer.fromString(data);
		data = new Uint8Array(data);
		const quality = flags?.quality;
		let payload = topic.length + data.length + (quality ? 2 : 0);

		let length = 1;	// PUBLISH
		length += getRemainingLength(payload);
		length += payload;

		let msg = new Uint8Array(length), position = 0, header = 0;
		if (flags) {
			if (flags.retain)
				header |= 1; 
			if (quality)
				header |= (quality & 3) << 1; 
			if (flags.duplicate)
				header |= 8; 
		}
		msg[position++] = PUBLISH | header;
		position = writeRemainingLength(payload, msg, position);
		msg.set(topic, position); position += topic.length;
		if (quality) {
			msg[position++] = this.packet >> 8;
			msg[position++] = this.packet;
		}
		msg.set(data, position); position += data.length;

		this.ws.write(msg.buffer);
	}
	subscribe(topic) {
		if (this.state < 2)
			throw new Error("connection closed");

		++this.packet;

		topic = makeStringBuffer(topic);
		let payload = topic.length + 1 + 2;

		let length = 1;	// SUBSCRIBE
		length += getRemainingLength(payload);
		length += payload;

		let msg = new Uint8Array(length), position = 0;
		msg[position++] = SUBSCRIBE | 2;		// SUBSCRIBE + flag
		position = writeRemainingLength(payload, msg, position);
		msg[position++] = (this.packet >> 8) & 0xFF;
		msg[position++] = this.packet & 0xFF;
		msg.set(topic, position); position += topic.length;
		// trailing 0 for QoS already in buffer

		this.ws.write(msg.buffer);
	}
	unsubscribe(topic) {
		if (this.state < 2)
			throw new Error("connection closed");

		++this.packet;

		topic = makeStringBuffer(topic);
		let payload = topic.length + 2;

		let length = 1;	// UNSUBSCRIBE
		length += getRemainingLength(payload);
		length += payload;

		let msg = new Uint8Array(length), position = 0;
		msg[position++] = UNSUBSCRIBE | 2;		// UNSUBSCRIBE + flag
		position = writeRemainingLength(payload, msg, position);
		msg[position++] = (this.packet >> 8) & 0xFF;
		msg[position++] = this.packet & 0xFF;
		msg.set(topic, position); position += topic.length;

		this.ws.write(msg.buffer);
	}
	received(buffer) {
		const length = buffer.byteLength;
		let position = 0;
		buffer = new Uint8Array(buffer);
		let parse = this.parse;
		while (position < length) {
			switch (parse.state) {
				case 0:
					parse.flags = buffer[position] & 0x0F;
					parse.code = buffer[position] & 0xF0;
					if ((0x00 === parse.code) || (0xF0 === parse.code))
						return this.fail("invalid code");

					position += 1;
					parse.state = 1;
					break;

				// remaining length
				case 1:
					parse.state = buffer[position] & 0x80 ? 2 : parse.code;
					parse.length = buffer[position++] & 0x7F;
					break;

				case 2:
					parse.state = buffer[position] & 0x80 ? 3 : parse.code;
					parse.length += (buffer[position++] & 0x7F) << 7;
					break;

				case 3:
					parse.state = buffer[position] & 0x80 ? 4 : parse.code;
					parse.length += (buffer[position++] & 0x7F) << 14;
					break;

				case 4:
					if (buffer[position] & 0x80)
						return this.fail("bad data");
					parse.length += (buffer[position++] & 0x7F) << 21;
					parse.state = parse.code;
					break;

				// CONNECT
				case 0x10:
					if (!parse.expect)
						parse.expect = Array.of(0, 4, 77, 81, 84, 84, 4);
					if (buffer[position++] !== parse.expect.shift())
						return this.fail("bad connect");
					if (!parse.expect.length) {
						delete parse.expect;
						parse.state = 0x11;
					}
					break;

				case 0x11:
					parse.flags = buffer[position++];
					if (parse.flags & 1)
						return this.fail("reserved not zero");
					parse.state = 0x12;
					break;

				case 0x12:
					parse.connect = {keepalive: buffer[position++] << 8};
					parse.state = 0x13;
					break;

				case 0x13:
					parse.connect.keepalive |= buffer[position++];
					parse.state = 0x14;
					parse.length -= 10;
					parse.buffer = new Uint8Array(parse.length);
					parse.buffer.position = 0;
					parse.state = 0x14;
					break;

				case 0x14:
					parse.buffer[parse.buffer.position++] = buffer[position++];
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
						const result = this.dispatch(parse);
						this.ws.write(Uint8Array.of(0x20, 2, 1, result).buffer);		// CONNACK
						parse = this.parse = {state: 0};
					}
					break;

				// PUBLISH
				case 0x30:
					parse.topic = buffer[position++] << 8;
					parse.state = 0x31;
					break;

				case 0x31:
					parse.topic |= buffer[position++];
					parse.length -= parse.topic + 2;
					if (0 === parse.topic)
						return this.fail("empty topic");
					parse.topic = new Uint8Array(parse.topic);
					parse.topic.position = 0;
					parse.state = 0x32;
					break;

				case 0x32:		// optimization: consume as many bytes as possible (see state 0x37)
					parse.topic[parse.topic.position++] = buffer[position++];
					if (parse.topic.position === parse.topic.length) {
						parse.topic = String.fromArrayBuffer(parse.topic.buffer);
						parse.state = 0x33;
					}
					else
						break;

				case 0x33:
					parse.state = (parse.flags & 0x06) ? 0x34 : 0x36;
					if (0x34 !== parse.state)
						break;

				case 0x34:
				case 0x35:
					position += 1;
					parse.length -= 1;
					parse.state += 1;
					if (0x36 !== parse.state)
						break;

				case 0x36:
					parse.payload = new Uint8Array(parse.length);
					parse.payload.position = 0;
					parse.state = 0x37;
					// fall through

				case 0x37:
					if (parse.length) {
						let use = parse.payload.length - parse.payload.position;
						if (use > (length - position))
							use = length - position;
						parse.payload.set(new Uint8Array(buffer.buffer, position, use), parse.payload.position);
						parse.payload.position += use;
						position += use;
					}
					if (parse.payload.position === parse.payload.length) {
						parse.payload = parse.payload.buffer;
						this.dispatch(parse);
						parse = this.parse = {state: 0};
					}
					break;

				// CONNACK
				case 0x20:
					if (2 !== parse.length)
						return this.fail("unexpected length");
					position += 1;	// connect acknowledge flags
					parse.length -= 1;
					parse.state = 0x21;
					break;

				case 0x21:
					parse.returnCode = buffer[position];
					parse.state = 0x22;
					// fall through

				case 0x22:
					position += 1;
					parse.length -= 1;
					if (0 === parse.length) {
						this.dispatch(parse);
						parse = this.parse = {state: 0};
					}
					break;

				case 0x80:		// SUBSCRIBE
				case 0xA0:		// UNSUBSCRIBE
					parse.id = buffer[position++] << 8;				// MSB packet ID
					parse.state = 0x81;
					break;

				case 0x81:
					parse.id |= buffer[position++];					// LSB packet ID
					parse.length -= 2;
					parse.buffer = new Uint8Array(parse.length);	// capture payload bytes
					parse.buffer.position = 0;
					parse.state = 0x82;
					break;

				case 0x82:
					parse.buffer[parse.buffer.position++] = buffer[position++];
					if (parse.buffer.position === parse.length) {
						const result = (0x80 === parse.code) ? [] : null;
						parse.buffer.position = 0;
						while (parse.buffer.position < parse.buffer.length) {
							parse.topic = String.fromArrayBuffer(getBuffer(parse.buffer));
							if (result) {
								parse.qos = parse.buffer[parse.buffer.position++];
								result.push(this.dispatch(parse) ?? 0);
							}
							else
								this.dispatch(parse)
						}
						if (result) {
							this.ws.write(Uint8Array.of(0x90, result.length + 2, parse.id >> 8, parse.id & 0xFF).buffer);	// SUBACK
							this.ws.write(Uint8Array.from(result).buffer);
						}
						else
							this.ws.write(Uint8Array.of(0xB0, 2, parse.id >> 8, parse.id & 0xFF).buffer);					// UNSUBACK
						parse = this.parse = {state: 0};
					}
					break;

				// PUBACK, PUBREC, PUBREL, PUBCOMP, SUBACK, UNSUBACK
				case 0x40:
				case 0x50:
				case 0x60:
				case 0x70:
				case 0x90:
				case 0xB0:		// MSB packet ID
					parse.id = buffer[position] << 8;
					position += 1;
					parse.length -= 1;
					parse.state = 0x91;
					break;

				case 0x91:		// LSB packet ID
					parse.id |= buffer[position];
					parse.state = 0x92;
					// fall through

				case 0x92:		// ignore remaining bytes
					position += 1;
					parse.length -= 1;
					if (0 === parse.length)
						parse = this.parse = {state: 0};		// not dispatched
					break;

				case 0xC0:		// PINGREQ
					this.ws.write(Uint8Array.of(0xD0, 0x00).buffer);		// PINGRESP
					parse = this.parse = {state: 0};
					break;

				case 0xD0:		// PINGRESP (ignored)
					parse = this.parse = {state: 0};
					break;

				case 0xE0:		// DISCONNECT
					debugger;		//@@ mosquitto_pub doesn't transmit this?
					break;

				default:
					return this.fail("bad parse state");
			}
		}

		if (this.#timer)
			this.last = Date.now();
	}
	connected() {
		this.onConnected?.();

		this.parse = {state: 0};
		this.packet = 1;

		if (this.server)
			return;

		const timeout = Math.floor(((this.timeout ?? 0) + 999) / 1000);
		const header = Uint8Array.of(
			0x00, 0x04,
			77,  81, 84, 84,						// protocol name MQTT
			0x04,									// protocol level 4 (MQTT version 3.1.1)
			0x02,									// flags : CleanSession
			(timeout >> 8) & 0xFF, timeout & 0xFF	// keepalive in seconds
		);
		const connect = this.connect;
		const id = makeStringBuffer(connect.id);
		const user = makeStringBuffer(connect.user);
		const password = makeStringBuffer(connect.password);
		let will = new Uint8Array(0);
		if (connect.will) {
			const topic = makeStringBuffer(connect.will.topic);
			let payload = connect.will.message;
			if (!(payload instanceof ArrayBuffer))
				payload = ArrayBuffer.fromString(payload);
			payload = new Uint8Array(payload);
			will = new Uint8Array(topic.length + 2 + payload.length);
			will.set(topic, 0);
			will[topic.length] = payload.length >> 8;
			will[topic.length + 1] = payload.length & 0xFF;
			will.set(payload, topic.length + 2);
		}
		const payload = header.length + id.length + user.length + password.length + will.length;

		let length = 1;	// CONNECT
		length += getRemainingLength(payload);
		length += payload;

		let msg = new Uint8Array(length), position = 0;
		msg[position++] = 0x10;
		position = writeRemainingLength(payload, msg, position);

		msg.set(header, position);
		msg[position + 7] |= (user.length ? 0x80 : 0) | (password.length ? 0x40 : 0) | (will.length ? 0x04 : 0) | ((will.length && connect.will.retain) ? 32 : 0);
		if (will.length)
			msg[position + 7] |= ((connect.will.quality ?? 0) & 3) << 3; 
		position += header.length;

		msg.set(id, position); position += id.length;
		msg.set(will, position); position += will.length;
		msg.set(user, position); position += user.length;
		msg.set(password, position); position += password.length;

		if (timeout) {
			this.#timer = Timer.repeat(this.keepalive.bind(this), this.timeout >> 2);
			this.last = Date.now();
		}

		delete this.connect;
		return msg.buffer;
	}
	dispatch(msg) {
		if (this.state <= 0)
			return;

		if (1 === this.state) {
			if (msg.code !== CONNACK)
				return this.fail(`received message type '${msg.code}' when expecting CONNACK`);

			if (msg.returnCode)
				return this.fail(`server rejected mqtt request with code ${msg.returnCode}`);

			this.state = 2;
		}

		this.#messages ??= [];
		this.#messages.push(msg);
		this.#messages.timer ??= Timer.set(() => {
			const messages = this.#messages;
			this.#messages = undefined; 
			for (let i = 0; i < messages.length; i++) {
				const msg = messages[i];
				try {
					switch (msg.code) {
						case CONNECT:
							this.onAccept(msg.connect);
							break;
						case CONNACK:
							this.onReady();
							break;
						case SUBSCRIBE:
							this.onSubscribe?.(msg.topic, msg.qos);
							break;
						case UNSUBSCRIBE:
							this.onUnsubscribe?.(msg.topic);
							break;
						case PUBLISH:
							this.onMessage?.(msg.topic, msg.payload);
							break;
						default:
							if (msg.code)
								this.fail(`unhandled or no-op message type '${msg.code}'`);
							break;
					}
				}
				catch {
				}
			}
		});
	}
	close(immediate) {
		if (this.ws) {
			try {
				if (!immediate)
					this.ws.write(Uint8Array.of(0xE0, 0x00).buffer);		 // just shoot it out there, don't worry about ACKs
			}
			catch {
			}
			try {
				this.ws.close();		// can throw if already closed (wi-fi disconnect notification not yet received )
			}
			catch {
			}
			delete this.ws;
		}

		Timer.clear(this.#timer);
		this.#timer = undefined;
		
		Timer.clear(this.#messages?.timer);
		this.#messages = undefined;

		this.state = -1;
	}
	keepalive() {
		const now = Date.now();
		if ((this.last + this.timeout) > now)
			return;		// received data within the timeout interval

		if ((this.last + (this.timeout + (this.timeout >> 1))) > now) {
			try {
				this.ws.write(Uint8Array.of(0xC0, 0x00).buffer);		// ping
			}
			catch {
				this.fail("write failed");
			}
		}
		else
			this.fail("time out"); // timed out after 1.5x waiting
	}
	fail(msg = "") {
		trace("MQTT FAIL: ", msg, "\n");
		this.onClose?.();
		this.close();
	}
}

function ws_callback(state, message) {
	switch (state) {
		case 1: // socket connected
			// we don't care about this, we only care when websocket handshake is done
			break;

		case 2: // websocket handshake complete
			// at this point we need to begin the MQTT protocol handshake
			this.state = 1;
			this.ws.write(this.connected());
			break;

		case 3: // message received
			try {
				return this.received(message);
			}
			catch {
				this.fail();
			}
			break;

		case 4: // websocket closed
			delete this.ws;
			this.onClose?.();
			this.close();
			break;

		default:
			this.fail(`unhandled websocket state ${state}`);
			break;
	}
}

function socket_callback(state, message) {
	switch (state) {
		case 1: // socket connected
			// at this point we need to begin the MQTT protocol handshake
			this.state = this.server ? 2 : 1;
			const reply = this.connected();
			if (reply)
				this.ws.write(reply);
			break;

		case 2: // data received
			try {
				return this.received(this.ws.read(ArrayBuffer));
			}
			catch {
				this.fail();
			}
			break;

		case 3: // ready to send
			break;

		default:	
			if (state < 0) {
				try {
					this.ws.close();
				}
				catch {
				}
				delete this.ws;
				this.fail(`socket error ${state}`);
			}
			else
				this.fail(`unhandled socket state ${state}`);
			break;
	}
}

function makeStringBuffer(string) {
	if (undefined === string)
		return new Uint8Array(0);

	if (!(string instanceof ArrayBuffer))
		string = ArrayBuffer.fromString(string);
	string = new Uint8Array(string);
	let result = new Uint8Array(string.length + 2);
	result[0] = string.length >> 8;
	result[1] = string.length & 0xFF;
	result.set(string, 2)

	return result;
}

function getRemainingLength(length) {
	if (length < 128)
		return 1;
	if (length < 16384)
		return 2;
	if (length < 2097152)
		return 3;
	return 4;
}

function writeRemainingLength(length, buffer, position) {
	if (length < 128)
		buffer[position++] = length;
	else if (length < 16384) {
		buffer[position++] = 0x80 | (length & 0x7F);
		buffer[position++] = length >> 7;
	}
	else if (length < 2097152) {
		buffer[position++] = 0x80 | (length & 0x7F);
		buffer[position++] = 0x80 | ((length >> 7) & 0x7F)
		buffer[position++] = length >> 14;
	}
	else {
		buffer[position++] = 0x80 | (length & 0x7F);
		buffer[position++] = 0x80 | ((length >> 7) & 0x7F)
		buffer[position++] = 0x80 | ((length >> 14) & 0x7F)
		buffer[position++] = length >> 21;
	}

	return position;
}

function getBuffer(buffer) {
	const position = buffer.position;
	const length = (buffer[position] << 8) | buffer[position + 1];
	const result = buffer.buffer.slice(position + 2, position + 2 + length);
	buffer.position += 2 + length;
	return result;
}
