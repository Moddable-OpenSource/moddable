/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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

	to do:
		
		collisions on id
		spread out sends
*/

import DNS from "dns";
import Parser from "dns/parser";
import Serializer from "dns/serializer";
import Timer from "timer";

class Resolver {
	#socket;
	#requests = [];
	#servers;
	#UDP;
	#timer;

	constructor(options) {
		this.#servers = options.servers.slice();
		this.#UDP = options.socket;
	}
	close() {
		this.#socket?.close();
		Timer.clear(this.#timer);
		this.#socket = this.#servers = this.#timer = this.#UDP = undefined;
	}
	resolve(options) {
		let {onResolved, onError, host} = options;
		if (!host || (!onResolved && !onError) || !this.#UDP)
			throw new Error;

		host = host.toString();
		if ("localhost" === host)
			host = "127.0.0.1";
		
		const parts = host.split(".");
		let isAddress = 4 === parts.length;
		if (isAddress) {
			for (let i = 0; i < 4; i++) {
				if (parseInt(parts[i]) != parts[i])
					isAddress = false; 
			}
		}
		const request = {
			state: 0,
			id: host.endsWith(".local") ? 0 : ((Math.random() * 65535) | 1),		//@@ collision
			host,
			onResolved,
			onError,
			isAddress
		}
		this.#requests.push(request);
		this.#timer ??= Timer.set(() => this.#task(), 0, 1000);
	} 
	#send(request) {
		const packet = new Serializer({query: true, recursionDesired: true, opcode: DNS.OPCODE.QUERY, id: request.id});
		packet.add(DNS.SECTION.QUESTION, request.host, DNS.RR.A, DNS.CLASS.IN);

		try {
			this.#timer ??= Timer.repeat(() => this.#task(), 1000);

			this.#socket ??= new (this.#UDP.io)({
				target: this,
				onReadable: function(count) {
					while (count--)
						this.target.#receive(this.read());
				}
			});

			if (request.id) {
				const servers = this.#servers;
				this.#socket.write(servers[request.state % servers.length], 53, packet.build());
			}
			else
				this.#socket.write("224.0.0.251", 5353, packet.build());
		}
		catch {
			if (!this.#timer) {
				this.#remove(request);
				request.onError?.call(this, request.host);
			}
		}
	}
	#receive(buffer) {
		const packet = new Parser(buffer);
		const question = packet.question(0);
		if (question && (DNS.CLASS.IN !== question.qclass))
			return;

		const id = packet.id;
		for (let i = 0, requests = this.#requests; i < requests.length; i++) {
			const request = requests[i];
			if (id !== request.id)
				continue;

			for (let j = 0, answers = packet.answers; j < answers; j++) {
				const answer = packet.answer(j);
				if (DNS.RR.A !== answer.qtype)
					continue;

				if ((question && (question.qname.join(".") === request.host)) || (answer.qname.join(".") === request.host)) {
					this.#remove(request);
					return request.onResolved?.call(this, request.host, answer.rdata);
				}
			}

			if (id) {
				this.#remove(request);
				return request.onError?.call(this, request.host);
			}
		}
	}
	#remove(request) {
		const requests = this.#requests;

		for (let i = 0; i < requests.length; i++) {
			if (requests[i] === request) {
				requests.splice(i, 1);
				break;
			}
		}
		if (!requests.length) {
			this.#socket?.close();
			Timer.clear(this.#timer);
			this.#socket = this.#timer = undefined;
		}
	}
	#task() {
		for (let i = 0, requests = this.#requests, servers = this.#servers; i < requests.length; i++) {
			const request = requests[i];
			if (request.isAddress) {
				this.#remove(request);
				request.onResolved?.call(this, request.host, request.host);
				i -= 1;
				continue;
			}
			const end = request.id ? servers.length * 5 : 5;
			request.state += 1;
			if (end === request.state) {
				this.#remove(request);
				request.onError?.call(this, request.host);
				i -= 1;
				continue;
			}
			this.#send(request);
		}
	}
 }

export default Resolver;
