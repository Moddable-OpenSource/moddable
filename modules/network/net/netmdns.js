/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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

// share one socket for multiple outstanding requests

import {Socket} from "socket";
import Parser from "dns/parser";
import Timer from "timer";	//@@ retry and timeout

export default class {
	static get() @ "xs_net_get";
	
	static resolve(host, client) {
		if (!host.endsWith(".local"))
			return this._resolve(host, client);

		const socket = new Socket({kind: "UDP"});
		socket.host = host;
		socket.client = client;
		socket.callback = callback;

		host = host.split(".");
		if (2 !== host.length)
			throw new Error("unrecognized");

		host[0] = ArrayBuffer.fromString(host[0]);
		host[1] = ArrayBuffer.fromString(host[1]);

		const packet = new Uint8Array(12 + host[0].byteLength + host[1].byteLength + 3 + 4);
		packet[5] = 1;		// 1 question
		let position = 12;
		packet[position++] = host[0].byteLength;
		packet.set(new Uint8Array(host[0]), position); position += host[0].byteLength;
		packet[position++] = host[1].byteLength;
		packet.set(new Uint8Array(host[1]), position); position += host[1].byteLength;
		packet[position + 2] = packet[position + 4] = 1;
		packet[position + 3] = 0x80;

		let count = 3;
		socket.timer = Timer.repeat(function (id) {
			if (0 === count--) {
				socket.client(socket.host)
				Timer.clear(id);
				return socket.close();
			}
			trace(`query ${socket.host}\n`);
			socket.write("224.0.0.251", 5353, packet.buffer);
		}, 1000, 1);
	}
	static _resolve(host, callback) @ "xs_net_resolve";
}

function callback(message, value, address, port)
{
	const packet = new Parser(this.read(ArrayBuffer));
	for (let i = 0; i < packet.answers; i++) {
		const answer = packet.answer(i);
		if (answer.qname.join(".") === this.host) {
			this.client(this.host, answer.rdata);
			break;
		}
	}

	Timer.clear(this.timer)
	this.close();
}
