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

const MDNS_TYPE_AAAA = 0x001C;
const MDNS_TYPE_A = 0x0001;
const MDNS_TYPE_PTR = 0x000C;
const MDNS_TYPE_SRV = 0x0021;
const MDNS_TYPE_TXT = 0x0010;

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
		this.write(MDNS_IP, MDNS_PORT, this.reply(0x0F, service, 0, true));		// remove before adding to clear stale state in clients
		service.timer = Timer.repeat(timer => {
			this.write(MDNS_IP, MDNS_PORT, this.reply(0x0F, service));
			timer.interval *= 2;
			if (timer.interval > 32000) {
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

		this.write(MDNS_IP, MDNS_PORT, this.reply(0x04, service));
	}
	remove(service) {
		const index = this.services.indexOf(service);
		if (index < 0) throw new Error("service not found");

		this.services.splice(index, 1);
		this.write(MDNS_IP, MDNS_PORT, this.reply(0x0F, service, 0, true));

		if (service.timer) {
			Timer.clear(service.timer);
			delete service.timer;
		}
	}

	callback(message, value, address, port) {
		const packet = new MDNS.Packet(this.read(ArrayBuffer));
		if (packet.flags & 0x8000)
			return;

		let mask = 0, service;

		for (let i = 0; i < packet.questions; i++) {
			const question = packet.question(i);
			const name = question.qname;

			switch (question.qtype) {
					// reply to name lookup query
				case MDNS_TYPE_A:
					if ((2 === name.length) && (LOCAL === name[1]) && (this.hostName == name[0]))
						mask |= 1;
					break;

				case MDNS_TYPE_PTR:
					if ((3 !== name.length) || (LOCAL !== name[2])) {
						if ((4 !== name.length) || ("_services" !== name[0]) || ("_dns-sd" !== name[1]) || ("_udp" !== name[2]) || (LOCAL !== name[3]))
							break;

						// reply to _services._dns_sd._udp.local query
						let respond = this.services.length;
						for (let j = 0; (j < packet.answers) && respond; j++) {
							let answer = packet.answer(j);
							if (MDNS_TYPE_PTR !== answer.qtype)
								continue;

							for (let k = 0; k < this.services.length; k++) {
								let service = this.services[k];
								if ((3 !== answer.rdata.length) || (("_" + service.name) !== answer.rdata[0]) ||
								   (("_" + service.protocol) !== answer.rdata[1]) || (LOCAL !== answer.rdata[2]))
								   continue;
								respond -= 1;
							}
						}
						if (respond) {
							let reply = []

							// if responding for any service, respond with all service types
							for (let k = 0; k < this.services.length; k++) {
								let service = this.services[k];
								reply.push(name[0], name[1], name[2], name[3], 0);

								const rdataLen = (service.name.length + 1) + (service.protocol.length + 1) + LOCAL.length + 4; // 4 is three label sizes and the terminator
								reply.push(Uint8Array.of(0, 0x0C, 0, 1, 0, 0, 0x11, 0x94, 0, rdataLen));

								reply.push("_" + service.name, "_" + service.protocol, LOCAL, 0);
							}
							this.write(address, port, this.serializePacket(packet.id, this.services.length, reply));	// unicast reply seems the norm (?)
						}
						break;
					}

					// reply to service query
					this.services.some(item => {
						if ((("_" + item.name) !== name[0]) || (("_" + item.protocol) !== name[1]))
							return;
						service = item;
						for (let j = 0; (j < packet.answers) && service; j++) {
							let answer = packet.answer(j);
							if (MDNS_TYPE_PTR !== answer.qtype)
							   continue;
						   if ((3 !== answer.qname.length) || (("_" + service.name) !== answer.qname[0]) ||
							   (("_" + service.protocol) !== answer.qname[1]) || (LOCAL !== answer.qname[2]))
							   continue;
						   if ((4 !== answer.rdata.length) || (this.hostName !== answer.rdata[0]) || (("_" + service.name) !== answer.rdata[1]) ||
							   (("_" + service.protocol) !== answer.rdata[2]) || (LOCAL !== answer.rdata[3]))
							   continue;
						   service = undefined;
					   }
						if (service)
							mask |= 0x0f;
						return true;
					});
					break;
			}
		}

		if (mask)
			this.write(MDNS_IP, port, this.reply(mask, service, packet.id));
	}

	reply(mask, service, id = 0, bye = false) {
		bye = bye ? 0 : 0xFF;
		const answerCount = (mask & 1) + ((mask & 2) >> 1) + ((mask & 4) >> 2) + ((mask & 8) >> 3);
		const packet = [];

		if (8 & mask) {		// PTR
			packet.push("_" + service.name, "_" + service.protocol, LOCAL, 0);

			// the type, class, ttl and rdata length
			let ptrDataLen = this.instanceName.length + (service.name.length + 1) + (service.protocol.length + 1) + 5 + 5; // 5 is four label sizes and the terminator
			packet.push(Uint8Array.of(0, 0x0C, 0, 1, 0, 0, 0x11 & bye, 0x94 & bye, 0, ptrDataLen));

			// the RData (ie. "My IOT device._http._tcp.local")
			packet.push(this.instanceName, "_" + service.name, "_" + service.protocol, LOCAL, 0);
		}

		if (4 & mask) {	// TXT
			packet.push(this.instanceName, "_" + service.name, "_" + service.protocol, LOCAL, 0);

			//Send the type, class, ttl and rdata length
			let length = 0;
			if (service.txt) {
				for (let property in service.txt)
					length += property.length + 1 + service.txt[property].toString().length + 1;
			}
			packet.push(Uint8Array.of(0, 0x10, 0x80, 1, 0, 0, 0x11 & bye, 0x94 & bye, 0, length));

			//Send the RData
			if (length) {
				for (let property in service.txt)
					packet.push(property + "=" + service.txt[property]);
			}
		}

		if (2 & mask) {	// SRV
			packet.push(this.instanceName, "_" + service.name, "_" + service.protocol, LOCAL, 0);

			let srvDataSize = this.hostName.length + 5 + 3; // 3 is 2 lable size bytes and the terminator
			srvDataSize += 6; // Size of Priority, weight and port
			packet.push(Uint8Array.of(0, 0x21, 0x80, 1, 0, 0, 0 & bye, 0x78 & bye, 0, srvDataSize));
			packet.push(Uint8Array.of(0, 0, 0, 0, service.port >> 8, service.port & 255));

			//RData (ie. "esp8266.local")
			packet.push(this.hostName, LOCAL, 0);
		}

		if (1 & mask) {	// A
			packet.push(this.hostName, LOCAL, 0);
			packet.push(Uint8Array.of(0, 1, 0x80, 1, 0, 0, 0 & bye, 0x78 & bye, 0, 4));
			// RData
			let ip = Net.get("IP").split(".");
			packet.push(Uint8Array.from(ip.map(value => parseInt(value))));
		}

		return this.serializePacket(id, answerCount, packet);
	}
	serializePacket(id, answerCount, packet) {
		packet.unshift(Uint8Array.of(id >> 8, id & 255, 0x84, 0, 0, 0, 0, answerCount, 0, 0, 0, 0));

		let position = packet.reduce((position, value) => {
			const type = typeof value;
			if ("number" === type)
				return position + 1;
			if ("string" === type)
				return position + 1 + value.length;
			return position + value.byteLength;
		}, 0);

		let buffer = new Uint8Array(position);
		position = 0;
		packet.forEach(value => {
			const type = typeof value;
			if ("number" === type) {
				buffer[position] = value;
				position += 1;
			}
			else if ("string" === type) {
				buffer[position++] = value.length;
				for (let i = 0; i < value.length; i++)
					buffer[position++] = value.charCodeAt(i);
			}
			else {
				buffer.set(value, position);
				position += value.byteLength;
			}
		});

		return buffer.buffer;
	}
}
Object.freeze(MDNS.prototype);

/*
function dumpPacket(packet)
{
	trace("\n", "** packet from ", address, " **\n");
	trace(" ID: ", packet.id.toString(16), ", FLAGS: ", packet.flags.toString(16), "\n");

	for (let i = 0; i < packet.questions; i++) {
		let q = packet.question(i);
		trace("  Q: ");
		trace(q.qname.join("."), ", ");
		trace("QTYPE: ", q.qtype.toString(16), ", ");
		trace("QCLASS: ", q.qclass.toString(16), "\n");
	}

	for (let i = 0; i < packet.answers; i++) {
		let a = packet.answer(i);
		trace("  A: ");
		trace(a.qname.join("."), ", ");
		trace("QTYPE: ", a.qtype.toString(16), ", ");
		trace("QCLASS: ", a.qclass.toString(16), ", ");
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

class Packet {
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
Object.freeze(Packet.prototype);
MDNS.Packet = Packet;

export default MDNS;
