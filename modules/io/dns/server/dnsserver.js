/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

import DNS from "dns";
import Parser from "dns/parser";
import Serializer from "dns/serializer";

class Server {
	#socket;
	#onResolve;
	#ttl;

	constructor(options) {
		this.#ttl = options.ttl ?? 60;
		this.#onResolve = options.onResolve;
		if (!this.#onResolve)
			throw new Error("onResolve required");

		const port = options.port ?? 53;
		this.#socket = new (options.socket.io)({
			target: this,
			port,
			onReadable(count) {
				while (count--) {
					const buffer = this.read();
					this.target.#receive(buffer);
				}
			}
		});
	}
	close() {
		this.#socket?.close();
		this.#socket = this.#onResolve = undefined;
	}
	#receive(buffer) {
		try {
			const packet = new Parser(buffer);
			const question = packet.question(0);
			if (!question || ((DNS.CLASS.IN !== question.qclass) && (DNS.CLASS.ANY !== question.qclass)))
				return;

			const name = question.qname.join(".");
			const resolved = this.#onResolve?.(name);		// name resolution is type-independent
			const response = new Serializer({query: false, authoritative: true, id: packet.id});
			response.add(DNS.SECTION.QUESTION, name, question.qtype, question.qclass);		// echo the question as asked

			if (!resolved)
				response.responseCode = DNS.RCODE.NXDOMAIN;		// name not served
			else if ((DNS.RR.A === question.qtype) || (DNS.RR.ANY === question.qtype))
				response.add(DNS.SECTION.ANSWER, name, DNS.RR.A, DNS.CLASS.IN, this.#ttl, resolved);

			this.#socket.write(response.build(), buffer.address, buffer.port);
		}
		catch (e) {
			/* ignore bad packets */
		}
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export default Server;
