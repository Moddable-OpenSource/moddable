/*
 * Copyright (c) 2016-2020 Moddable Tech, Inc.
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

import {Socket} from "socket";

class Server extends Socket {
	constructor(callback, dictionary = {}) {
		super({kind: "UDP", port: dictionary.port ? dictionary.port : 53});
		this.client = callback;
		this.ttl = dictionary.ttl ? dictionary.ttl : 60;
	}
	callback(message, value, address, port) {
		if (2 == message) {
			let request = new Uint8Array(this.read(ArrayBuffer, value));
			const id = (request[0] << 8) | request[1];
			const flags = (request[2] << 8) | request[3];
			const QDCount = (request[4] << 8) | request[5];
			const ANCount = (request[6] << 8) | request[7];
			const NSCount = (request[8] << 8) | request[9];
			const ARCount = (request[10] << 8) | request[11];

			let name = [];
			let index = 12;
			while (true) {
				let length = request[index++];
				if (!length) break;

				let part = "";
				while (length--)
					part += String.fromCharCode(request[index++]);
				name.push(part);
			}
			let type = (request[index] << 8) | request[index + 1];
			if (1 !== type)
				return;		// only reply to A

			let result = this.client(1, name.join("."));
			if (result) {
				let response = new Uint8Array(request.byteLength + 12 + 4);

				for (let i = 0; i < request.byteLength; i++)
					response[i] = request[i];
				response[2] |= 0x80;		// QR = DNS_QR_RESPONSE
				response[6] = response[4], response[7] = response[5];		// ANCount = QDCount

				let index = request.byteLength;
				response[index++] = 192;
				response[index++] = 12;
				response[index++] = 0;
				response[index++] = 1;
				response[index++] = 0;
				response[index++] = 1;

				response[index++] = (this.ttl >> 24) & 0xff;
				response[index++] = (this.ttl >> 16) & 0xff;
				response[index++] = (this.ttl >> 8) & 0xff;
				response[index++] = this.ttl & 0xff;

				response[index++] = 0;
				response[index++] = 4;

				result = result.split(".");
				for (let i = 0; i < 4; i++)
					response[index++] = parseInt(result[i]);

				this.write(address, port, response.buffer);
			}
		}
	}
}
Server.resolve = 1;
Object.freeze(Server.prototype);

export default Server;
