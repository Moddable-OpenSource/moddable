/*
 * Copyright (c) 2020 Moddable Tech, Inc.
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

import Net from "net";
import DNS from "dns";
import Parser from "dns/parser";
import Serializer from "dns/serializer";
import SecureSocket from "securesocket";
import {Request} from "http";
import Timer from "timer";

/*
	note:
		- this module does no caching
		- requests are resolved sequentially to minimize peak memory use
*/

let manager = null;

class Manager {
	requests = [];
	request;

	add(request) {
		this.requests.push(request);
		if (1 === this.requests.length)
			this.send(request);
	}
	remove(request) {
		for (let i = 0; i < this.requests.length; i++) {
			if (this.requests[i].request === request) {
				this.requests.splice(i, 1);
				if (0 === i) {
					if (this.request)
						this.request.close();
					delete this.request;
				}
				break;
			}
		}
		if (!this.requests.length) {
			manager = null;
			if (this.request)
				this.request.close();
			delete this.request;
		}
	}
	send(request) {
		const body = new Serializer({query: true, recursionDesired: true, opcode: DNS.OPCODE.QUERY, id: request.id});
		body.add(DNS.SECTION.QUESTION, request.host, DNS.RR.A, DNS.CLASS.IN);

		this.request = new Request({
			host: "8.8.8.8",
			path: "/dns-query",
			port: 443,
			headers: ["host", "dns.google", "content-type", "application/dns-message"],
			method: "POST",
			body: body.build(),
			response: ArrayBuffer,
			Socket: SecureSocket,
			secure: {
				protocolVersion: 0x303,
				trace: false,
			}
		});
		this.request.callback = this.callback.bind(this);
	}
	callback(message, value, address, port) {
		let done;

		if (Request.responseComplete === message) {
			const packet = new Parser(value);
			try {
				const request = this.requests[0];
				if (packet.answers)
					request.onResolved.call(request.target, packet.answer(0).rdata);
				else
					request.onError.call(request.target);
			}
			catch {
			}

			done = true;
		}
		else if (Request.error === message)
			done = true;

		if (done) {
			delete this.request;
			this.requests.shift();
			if (this.requests.length)
				this.send(this.requests[0]);
		}
	}
}
Object.freeze(Manager.prototype);

class Resolver {
	constructor(options) {
		if (!manager)
			manager = new Manager;

		manager.add({
			request: this,
			host: options.host.toString(),
			target: options.target || this,
			onResolved: options.onResolved || this.onResolved,
			onError: options.onError || this.onError,
		});
	}
	close() {
		if (this.timer) {
			Timer.clear(this.timer);
			delete this.timer;
		}

		manager.remove(this);
	}
}
Object.freeze(Resolver.prototype);

export default Resolver;
