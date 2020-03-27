/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

export default class Client {
	constructor(dictionary) {
		if (!dictionary.id)
			throw new Error("parameter id is required");

		if (!dictionary.host)
			throw new Error("parameter host is required");

		this.connect = {id: dictionary.id};
		if (dictionary.user)
			this.connect.user = dictionary.user;
		if (dictionary.password)
			this.connect.password = dictionary.password;

//		// set default callbacks to be overridden by caller
//		this.onConnected = function() {};
//		this.onReady = function() {};
//		this.onMessage = function() {};
//		this.onClose = function() {};

		const path = dictionary.path ? dictionary.path : null; // includes query string

		this.state = 0;
		this.parse = {state: 0};

		this.packet = 1;

		if (dictionary.timeout)
			this.timeout = dictionary.timeout;

		if (this.path) {
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
	publish(topic, data) {
		if (this.state < 2)
			throw new Error("connection closed");

		++this.packet;

		topic = makeStringBuffer(topic);
		if (!(data instanceof ArrayBuffer))
			data = ArrayBuffer.fromString(data);
		data = new Uint8Array(data);
		let payload = topic.length + data.length + 0;

		let length = 1;	// PUBLISH
		length += getRemainingLength(payload);
		length += payload;

		let msg = new Uint8Array(length), position = 0;
		msg[position++] = 0x30;		// PUBLISH
		position = writeRemainingLength(payload, msg, position);
		msg.set(topic, position); position += topic.length;
		// packetID goes here if QoS > 0 (unimplemented)
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
		msg[position++] = 0x82;		// SUBSCRIBE + flag
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
		msg[position++] = 0xA2;		// UNSUBSCRIBE + flag
		position = writeRemainingLength(payload, msg, position);
		msg[position++] = (this.packet >> 8) & 0xFF;
		msg[position++] = this.packet & 0xFF;
		msg.set(topic, position); position += topic.length;

		this.ws.write(msg.buffer);
	}
	received(buffer) {
		let length = buffer.byteLength, position = 0;
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

				// PUBLISH
				case 0x30:
					parse.topic = buffer[position++] << 8;
					parse.state = 0x31;
					break;

				case 0x31:
					parse.topic = buffer[position++];
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

				case 0xD0:	// PINGRESP (ignored - N.B. this will not trigger until first byte of next message received)
					parse = this.parse = {state: 0};
					break;

				default:
					this.fail("bad parse state");
					break;
			}
		}

		if (this.timer)
			this.last = Date.now();
	}
	connected() {
		this.onConnected?.();

		const timeout = Math.floor(((this.timeout || 0) + 999) / 1000);
		const header = Uint8Array.of(
			0x00, 0x04,
			77,  81, 84, 84,						// protocol name MQTT
			0x04,									// protocol level 4 (MQTT version 3.1.1)
			0x02,									// flags : CleanSession
			(timeout >> 8) & 0xFF, timeout & 0xFF	// keepalive in seconds
		);
		const id = makeStringBuffer(this.connect.id);
		const user = makeStringBuffer(this.connect.user);
		const password = makeStringBuffer(this.connect.password);
		const payload = header.length + id.length + user.length + password.length

		let length = 1;	// CONNECT
		length += getRemainingLength(payload);
		length += payload;

		let msg = new Uint8Array(length), position = 0;
		msg[position++] = 0x10;
		position = writeRemainingLength(payload, msg, position);

		msg.set(header, position);
		msg[position + 7] |= (user.length ? 0x80 : 0) | (password.length ? 0x40 : 0);
		position += header.length;

		msg.set(id, position); position += id.length;
		msg.set(user, position); position += user.length;
		msg.set(password, position); position += password.length;

		if (timeout) {
			this.timer = Timer.repeat(this.keepalive.bind(this), this.timeout >> 2);
			this.last = Date.now();
		}

		delete this.connect;
		return msg.buffer;
	}
	dispatch(msg) {
		if (1 === this.state) {
			if (msg.code !== CONNACK)
				return this.fail(`received message type '${flag}' when expecting CONNACK`);

			if (msg.returnCode)
				return this.fail(`server rejected mqtt request with code ${msg.returnCode}`);

			this.state = 2;
			try {
				this.onReady();
			}
			catch {
			}
			return;
		}

		switch (msg.code) {
			case PUBLISH:
				try {
					this.onMessage(msg.topic, msg.payload);
				}
				catch {
				}
				break;
			default:
				if (msg.code)
					this.fail(`received unhandled or no-op message type '${msg.code}'`);
				break;
		}

	}
	close() {
		if (this.timer)
			Timer.clear(this.timer);
		delete this.timer;

		if (this.ws) {
			try {
				this.ws.write(Uint8Array.of(0xE0, 0x00).buffer);		 // just shoot it out there, don't worry about ACKs
			}
			catch {
			}
			this.ws.close();
			delete this.ws;
		}

		this.state = 0;
	}
	keepalive() {
		if (!this.timer)
			return;

		let now = Date.now();
		if ((this.last + this.timeout) > now)
			return;		// received data within the timeout interval

		if ((this.last + (this.timeout + (this.timeout >> 1))) > now)
			this.ws.write(Uint8Array.of(0xC0, 0x00).buffer);		// ping
		else
			this.fail("time out"); // timed out after 1.5x waiting
	}
	fail(msg = "") {
		trace("MQTT FAIL: ", msg, "\n");
		this.onClose?.();
		this.close();
	}
}
Object.freeze(Client.prototype);

const CONNACK = 0x20;
const PUBLISH = 0x30;

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
			return this.received(message);

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
			this.state = 1;
			this.ws.write(this.connected());
			break;

		case 2: // data received
			return this.received(this.ws.read(ArrayBuffer));

		case 3: // ready to send
			break;

		default:	
			if (state < 0) {
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
	else if (length < 16384)
		return 2;
	else if (length < 2097152)
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
