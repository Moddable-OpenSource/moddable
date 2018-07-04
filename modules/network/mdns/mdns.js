/*
 * Copyright (c) 2018 Moddable Tech, Inc.
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
import Net from "net";
import Timer from "timer";

const MDNS_TYPE_A = 1;
const MDNS_TYPE_PTR = 12;
const MDNS_TYPE_TXT = 16;
const MDNS_TYPE_AAAA = 28;
const MDNS_TYPE_SRV = 33;
const MDNS_TYPE_ANY = 255;

const MDNS_CLASS_IN = 0x0001;
const MDNS_NAME_REF = 0xC000;

const MDNS_IP = "224.0.0.251";
const MDNS_PORT = 5353;

const LOCAL = "local";

class MDNS extends Socket {
	constructor(dictionary) {
		super({kind: "UDP", port: MDNS_PORT, multicast: MDNS_IP});

		this.hostName = dictionary.hostName;
		this.instanceName = dictionary.instanceName ? dictionary.instanceName : this.hostName;
		this.services = [];
	}
	add(service) {
		this.services.push(service);
		this.write(MDNS_IP, MDNS_PORT, this.reply(null, 0x0F, service, true));		// remove before adding to clear stale state in clients
		service.timer = Timer.repeat(timer => {
			this.write(MDNS_IP, MDNS_PORT, this.reply(null, 0x1F, service));
			timer.interval *= 2;
			if (timer.interval > 16000) {
				Timer.clear(timer);
				delete service.timer;
			}
			else
				Timer.schedule(timer, timer.interval, timer.interval);
		}, 1000).interval = 500;
	}
	update(service) {
		const index = this.services.indexOf(service);
		if (index < 0) throw new Error("service not found");

		this.write(MDNS_IP, MDNS_PORT, this.reply(null, 0x04, service));
	}
	remove(service) {
		const index = this.services.indexOf(service);
		if (index < 0) throw new Error("service not found");

		this.services.splice(index, 1);
		this.write(MDNS_IP, MDNS_PORT, this.reply(null, 0x0F, service, true));

		if (service.timer) {
			Timer.clear(service.timer);
			delete service.timer;
		}
	}

	callback(message, value, address, port) {
		const packet = new MDNS.Parser(this.read(ArrayBuffer))
		let response;

//		dumpPacket(packet, address);
		if (packet.flags & 0x8000)
			return;

		for (let h = 0, length = this.services.length ? this.services.length : 1; h < length; h++) {	// always at least one pass, to pick up service-indepdent qtypes
			const service = this.services[h];
			let mask = 0;

			for (let i = 0; i < packet.questions; i++) {
				const question = packet.question(i);
				const name = question.qname;

				switch (question.qtype) {
					case MDNS_TYPE_A:
						if (h)
							;
						else if ((2 === name.length) && (LOCAL === name[1]) && (this.hostName == name[0]))
							mask |= 1;
						break;

					case MDNS_TYPE_PTR:
						if (!service)
							;
						else
						if ((3 === name.length) && (LOCAL === name[2])) {
							// service query
							if ((("_" + service.name) === name[0]) && (("_" + service.protocol) === name[1])) {
								let respond = true;

								for (let j = 0; (j < packet.answers) && respond; j++) {
									let answer = packet.answer(j);
									if (MDNS_TYPE_PTR !== answer.qtype)
									   continue;
								   if ((3 !== answer.qname.length) || (("_" + service.name) !== answer.qname[0]) ||
									   (("_" + service.protocol) !== answer.qname[1]) || (LOCAL !== answer.qname[2]))
									   continue;
								   if ((4 !== answer.rdata.length) || (this.hostName !== answer.rdata[0]) || (("_" + service.name) !== answer.rdata[1]) ||
									   (("_" + service.protocol) !== answer.rdata[2]) || (LOCAL !== answer.rdata[3]))
									   continue;
								   respond = false;
								}
								if (respond)
									mask |= 0x1f;
							}
						}
						else
						if ((4 === name.length) && ("_services" === name[0]) && ("_dns-sd" === name[1]) && ("_udp" === name[2]) && (LOCAL === name[3])) {
							let respond = true;
							for (let j = 0; (j < packet.answers) && respond; j++) {
								let answer = packet.answer(j);
								if (MDNS_TYPE_PTR !== answer.qtype)
									continue;

								if ((3 === answer.rdata.length) && (("_" + service.name) === answer.rdata[0]) &&
								   (("_" + service.protocol) === answer.rdata[1]) && (LOCAL === answer.rdata[2]))
								   respond = false;
							}
							if (respond)
								mask |= 0x10;
							break;
						}
						break;

					case MDNS_TYPE_SRV:
						if (!service)
							;
						else
						if ((4 === name.length) && (LOCAL === name[3]) && (this.hostName === name[0]) &&
							(("_" + service.name) === name[1]) && (("_" + service.protocol) === name[2]))
							mask |= 2;		// not checking known answers... seems to never be populated.
						break;

					case MDNS_TYPE_ANY:
						if ((2 === name.length) && (LOCAL === name[1]) && (this.hostName === name[0]))
							mask |= service ? 0x1f : 1;
						break;
				}
			}

			//@@ delay response 20-120 ms
			if (mask) {
				if (!response)
					response = new MDNS.Serializer;
				this.reply(response, mask, service)
			}
		}

		if (response)
			this.write(MDNS_IP, port, response.build(packet.id));		// could be unicast
	}

	/*
		16 - service_ptr
		8 - PTR
		4 - TXT
		2 - SRV
		1 - A
	 */
	reply(response, mask, service, bye = false) {
		let build = response ? false : true;
		if (build) response = new MDNS.Serializer;
		bye = bye ? 0 : 0xFF;

		if (16 & mask) {	// PTR for service discovery
			let answer = [];
			answer.push("_services", "_dns-sd", "_udp", LOCAL, 0);

			const rdataLen = (service.name.length + 1) + (service.protocol.length + 1) + LOCAL.length + 4; // 4 is three label sizes and the terminator
			answer.push(Uint8Array.of(0, 0x0C, 0, 1, 0, 0, 0x11, 0x94, 0, rdataLen));

			answer.push("_" + service.name, "_" + service.protocol, LOCAL, 0);
			response.add(answer);
		}

		if (8 & mask) {		// PTR
			let answer = [];
			answer.push("_" + service.name, "_" + service.protocol, LOCAL, 0);

			// the type, class, ttl and rdata length
			const rdataLen = this.instanceName.length + (service.name.length + 1) + (service.protocol.length + 1) + 5 + 5; // 5 is four label sizes and the terminator
			answer.push(Uint8Array.of(0, 0x0C, 0, 1, 0, 0, 0x11 & bye, 0x94 & bye, 0, rdataLen));

			// the RData (ie. "My IOT device._http._tcp.local")
			answer.push(this.instanceName, "_" + service.name, "_" + service.protocol, LOCAL, 0);
			response.add(answer);
		}

		if (4 & mask) {	// TXT
			let answer = [];
			answer.push(this.instanceName, "_" + service.name, "_" + service.protocol, LOCAL, 0);

			//Send the type, class, ttl and rdata length
			let rdataLen = 0;
			if (service.txt) {
				for (let property in service.txt)
					rdataLen += property.length + 1 + service.txt[property].toString().length + 1;
			}
			answer.push(Uint8Array.of(0, 0x10, 0x80, 1, 0, 0, 0x11 & bye, 0x94 & bye, 0, rdataLen ? rdataLen : 1));

			if (rdataLen) {
				for (let property in service.txt)
					answer.push(property + "=" + service.txt[property]);
			}
			else
				answer.push(0);		// mDNS requires at least one rdata payload
			response.add(answer);
		}

		if (2 & mask) {	// SRV
			let answer = [];
			answer.push(this.instanceName, "_" + service.name, "_" + service.protocol, LOCAL, 0);

			let rdataLen = this.hostName.length + 5 + 3; // 3 is 2 lable size bytes and the terminator
			rdataLen += 6; // Size of Priority, weight and port
			answer.push(Uint8Array.of(0, 0x21, 0x80, 1, 0, 0, 0 & bye, 0x78 & bye, 0, rdataLen));
			answer.push(Uint8Array.of(0, 0, 0, 0, service.port >> 8, service.port & 255));

			answer.push(this.hostName, LOCAL, 0);
			response.add(answer);
		}

		if (1 & mask) {	// A
			let answer = [];
			answer.push(this.hostName, LOCAL, 0);
			answer.push(Uint8Array.of(0, 1, 0x80, 1, 0, 0, 0 & bye, 0x78 & bye, 0, 4));
			// RData
			const ip = Net.get("IP").split(".");
			answer.push(Uint8Array.from(ip.map(value => parseInt(value))));
			response.add(answer);
		}

		if (!build)
			return;

		return response.build();
	}
}
Object.freeze(MDNS.prototype);

/*
function dumpPacket(packet, address)
{
	trace("\n", "** packet from ", address, " **\n");
	trace(" ID: ", packet.id.toString(16), ", FLAGS: ", packet.flags.toString(16), "\n");

	for (let i = 0; i < packet.questions; i++) {
		let q = packet.question(i);
		trace("  Q: ");
		trace(q.qname.join("."), ", ");
		trace("QTYPE: ", q.qtype.toString(), ", ");
		trace("QCLASS: 0x", q.qclass.toString(16), "\n");
	}

	for (let i = 0; i < packet.answers; i++) {
		let a = packet.answer(i);
		trace("  A: ");
		trace(a.qname.join("."), ", ");
		trace("QTYPE: ", a.qtype.toString(), ", ");
		trace("QCLASS: 0x", a.qclass.toString(16), ", ");
		trace("TTL: ", a.ttl.toString());
		if (a.rdata) {
			trace(", ", "RDATA: ");
			if (Array.isArray(a.rdata))
				trace(a.rdata.join("."));
			else if ("object" === typeof a.rdata)
				trace(JSON.stringify(a.rdata));
			else
				trace(a.rdata);
		}
		trace("\n");
	}
}
*/

class Parser {
	constructor(packet) {
		this.buffer = packet;
	}
	get id() @ "xs_mdnspacket_get_id"
	get flags() @ "xs_mdnspacket_get_flags"
	get questions() @ "xs_mdnspacket_get_questions"
	get answers() @ "xs_mdnspacket_get_answers"
	question(index) @ "xs_mdnspacket_question"
	answer(index) @ "xs_mdnspacket_answer"
}
Object.freeze(Parser.prototype);
MDNS.Parser = Parser;

class Serializer {
	constructor() {
		this.fragments = [];
	}
	add(parts) {
		const byteLength = parts.reduce((byteLength, value) => {
			const type = typeof value;
			if ("number" === type)
				return byteLength + 1;
			if ("string" === type)
				return byteLength + 1 + value.length;
			return byteLength + value.byteLength;
		}, 0);

		let fragment = new Uint8Array(byteLength);
		let position = 0;
		parts.forEach(value => {
			const type = typeof value;
			if ("number" === type) {
				fragment[position] = value;
				position += 1;
			}
			else if ("string" === type) {
				fragment[position++] = value.length;
				for (let i = 0; i < value.length; i++)
					fragment[position++] = value.charCodeAt(i);
			}
			else {
				fragment.set(value, position);
				position += value.byteLength;
			}
		});

		// check for duplicate answer
	fragments:
		for (let i = 0; i < this.fragments.length; i++) {
			const k = this.fragments[i];
			if (k.byteLength !== byteLength)
				continue;
			for (let j = 0; j < byteLength; j++) {
				if (fragment[j] !== k[j])
					continue fragments;
			}
			return;
		}
		this.fragments.push(fragment);
	}
	build(id = 0) {
		const byteLength = this.fragments.reduce((byteLength, value) => byteLength + value.byteLength, 0);
		let position = 12;
		let result = new Uint8Array(byteLength + position);
		result.set(Uint8Array.of(id >> 8, id & 255, 0x84, 0, 0, 0, 0, this.fragments.length, 0, 0, 0, 0), 0);		// header
		this.fragments.forEach(fragment => {
			result.set(fragment, position);
			position += fragment.byteLength;
		});
		return result.buffer;
	}
}
Object.freeze(Serializer.prototype);
MDNS.Serializer = Serializer;

export default MDNS;
