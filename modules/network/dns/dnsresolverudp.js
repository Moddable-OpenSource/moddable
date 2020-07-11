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
import {Socket} from "socket";
import Timer from "timer";

/*
	to do:
		- validate address packet received from is in DNS server list
		- choose random port for UDP

	note:
		- this module does no caching
		- requests are retried at 3 second intervals, up to three times for each DNS server
		- .local addresses resolved with mDNS
*/

let manager = null;

class Manager {
	requests = [];
	socket = new Socket({kind: "UDP"});

	constructor(options) {
		this.socket.callback = this.callback.bind(this);
		this.timer = Timer.repeat(this.task.bind(this), 1000);
	}
	add(request) {
		request.state = 0;
		request.id = request.host.endsWith(".local") ? 0 : ((Math.random() * 65535) | 1);
		this.requests.push(request);

		try {
			this.send(request);
		}
		catch {
		}
	}
	remove(request) {
		for (let i = 0; i < this.requests.length; i++) {
			if (this.requests[i].request === request) {
				this.requests.splice(i, 1);
				break;
			}
		}
		if (!this.requests.length) {
			manager = null;
			this.socket.close();
			Timer.clear(this.timer);
		}
	}
	send(request) {
		const packet = new Serializer({query: true, recursionDesired: true, opcode: DNS.OPCODE.QUERY, id: request.id});
		packet.add(DNS.SECTION.QUESTION, request.host, DNS.RR.A, DNS.CLASS.IN);

		if (request.id) {
			const dns = Net.get("DNS");
			this.socket.write(dns[request.state % dns.length], 53, packet.build());
		}
		else
			this.socket.write("224.0.0.251", 5353, packet.build());
	}
	callback(message /*, value, address, port */) {
		if (2 !== message)
			return;

		const packet = new Parser(this.socket.read(ArrayBuffer));
		const question = packet.question(0);
		if (question && (DNS.CLASS.IN !== question.qclass))
			return;
		const id = packet.id;
		for (let i = 0; i < this.requests.length; i++) {
			const request = this.requests[i];
			if (id !== request.id)
				continue;

			for (let j = 0, answers = packet.answers; j < answers; j++) {
				const answer = packet.answer(j);
				if (DNS.RR.A !== answer.qtype)
					continue;

				if ((question && (question.qname.join(".") === request.host)) || (answer.qname.join(".") === request.host)) {
					this.remove(request.request);
					return request.onResolved?.call(request.request, answer.rdata);
				}
			}

			if (id) {
				this.remove(request.request);
				return request.onError?.call(request.request);
			}
		}
	}
	task() {
		try {
			for (let i = 0, requests = this.requests; i < requests.length; i++) {
				const request = requests[i];
				request.state += 1;
				if (!(request.state % 3)) {
					if ((request.id ? (Net.get("DNS").length * 6) : 6) === request.state) {
						this.remove(request.request);
						request.onError?.call(request.request);
						i -= 1;
						continue;
					}
				}
				this.send(request);
			}
		}
		catch {
		}
	}
}
Object.freeze(Manager.prototype);

class Resolver {
	constructor(options) {
		manager ??= new Manager;

		manager.add({
			request: this,
			host: options.host.toString(),
			onResolved: options.onResolved,
			onError: options.onError,
		});

		if (options.target)
			this.target = options.target;
	}
	close() {
		manager?.remove(this);
	}
}
Object.freeze(Resolver.prototype);

export default Resolver;
