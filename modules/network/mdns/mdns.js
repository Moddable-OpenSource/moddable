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

/*
	To do:
		Proper TTL on Answer
		Refresh / expire monitored records
		case-insensitivity - publish unchanged (uppercase) compare case-insensitive
		instance name
		unicast reponse only to addresses on same subnet
*/

import {Socket} from "socket";
import Parser from "dns/parser";
import Serializer from "dns/serializer";
import DNS from "dns";
import Net from "net";
import Timer from "timer";

const LOCAL = "local";
const TTL = 4500;

class MDNS extends Socket {
	constructor(dictionary = {}, callback) {
		super({kind: "UDP", port: MDNS.PORT, multicast: MDNS.IP});

		this.services = [];
		this.monitors = [];
		this.client = callback ? callback : function() {};

		if (dictionary.hostName) {
			this.hostName = dictionary.hostName;
			this.probing = 1;
			Timer.set(() => this.probe(), 0);
		}
	}
	add(service) {
		if (!this.hostName)
			throw new Error("no hostName");

		if (this.probing)
			throw new Error("not ready");

		this.services.push(service);

		this.write(MDNS.IP, MDNS.PORT, this.reply(null, 0x1F | 0x8000, service, true));		// remove before adding to clear stale state in clients
		service.timer = Timer.repeat(timer => {
			this.write(MDNS.IP, MDNS.PORT, this.reply(null, 0x1F, service));
			timer.interval *= 2;
			if (timer.interval > 16000) {
				Timer.clear(timer);
				delete service.timer;
			}
			else
				Timer.schedule(timer, timer.interval, timer.interval);
		}, 500).interval = 500;
	}
	update(service) {
		const index = this.services.indexOf(service);
		if (index < 0) throw new Error("service not found");

		if (!service.update) {
			Timer.repeat(id => {
				this.write(MDNS.IP, MDNS.PORT, this.reply(null, 0x04 | 0x8000, service));
				service.update--;
				if (0 == service.update) {
					Timer.clear(id)
					delete service.update;
				}
			}, 250, 0);
		}
		service.update = 3;

	}
	remove(service) {
		if ("string" === typeof service) {
			service += ".local";
			service = this.monitors.findIndex(item => item.service === service);
			if (service >= 0) {
				Timer.clear(this.monitors[service].timer);
				this.monitors.splice(service, 1);
			}
		}
		else {
			const index = this.services.indexOf(service);
			if (index < 0) throw new Error("service not found");

			this.services.splice(index, 1);
			this.write(MDNS.IP, MDNS.PORT, this.reply(null, 0x0F, service, true));

			if (service.timer) {
				Timer.clear(service.timer);
				delete service.timer;
			}
		}
	}
	monitor(service, callback) {
		service += ".local";
		if (this.monitors.find(item => item === service))
			throw new Error("redundant");

		let results = [];
		results.service = service;
		results.state = 0;
		results.callback = callback;
		results.mdns = this;
		results.timer = Timer.set(this.monitorTask.bind(results), 1);
		this.monitors.push(results);
	}
	monitorTask() {
		let mdns = this.mdns;
		switch (this.state) {
			case 0:
			case 1:
			case 2:
			case 3:
				let query = new Serializer({query: true, opcode: DNS.OPCODE.QUERY});

				query.add(DNS.SECTION.QUESTION, this.service, DNS.RR.PTR, DNS.CLASS.IN | ((0 == this.state) ? MDNS.UNICAST : 0));

				for (let i = 0; i < this.length; i++)
					query.add(DNS.SECTION.ANSWER, this.service, DNS.RR.PTR, DNS.CLASS.IN, TTL, this[i].name + "." + this.service);

				mdns.write(MDNS.IP, MDNS.PORT, query.build());

				if (this.state < 3) {
					this.timer = Timer.set(mdns.monitorTask.bind(this), (2 ** this.state) * 1000);
					this.state += 1;
				}
				else
					this.timer = Timer.set(mdns.monitorTask.bind(this), 30 * 60 * 1000);		//@@ calculate based on TTL elapsed (section 5.2 Continuous Multicast DNS Querying)
				break;
		}
	}
	scanPacket(packet, address) {
		const answers = packet.answers;
		let changed;
		for (let i = 0, length = answers + packet.additionals; i < length; i++) {
			const record = (i < answers) ? packet.answer(i) : packet.additional(i - answers);
			let name = record.qname, service;
			let monitor, instance;

			switch (record.qtype) {
				case DNS.RR.A:
					//@@ if not probing, check to see if name matches and IP is different. If so there is a conflict, revert to probing state...
					if ((this.probing > 0) && (this.hostName == name[0]) && (2 === name.length) && (LOCAL === name[1])) {
						trace(`probe conflict with ${address}\n`);
						this.probing = -1;
						this.probeAttempt += 1;
						break;
					}
					name = name.join(".");
					this.monitors.forEach(monitor => {
						monitor.forEach(instance => {
						if (instance.target === name)
							instance.address = record.rdata;
						});
					});
					break;
				case DNS.RR.PTR:
					service = name.join(".");
					monitor = this.monitors.find(monitor => monitor.service === service);
					if (!monitor)
						break;
					instance = monitor.find(item => item.name === record.rdata[0]);
					if (!instance) {
						instance = {name: record.rdata[0]};
						monitor.push(instance);
						instance.changed = changed = true;
					}
					instance.ttl = record.ttl
					break;
				case DNS.RR.SRV:
				case DNS.RR.TXT:
					service = name.slice(1).join(".");
					monitor = this.monitors.find(monitor => monitor.service === service);
					if (!monitor)
						break;
					instance = monitor.find(item => item.name === name[0]);
					if (!instance) {
						instance = {name: name[0]};
						monitor.push(instance);
					}
					if (DNS.RR.TXT === record.qtype) {
						instance.txt = record.rdata ? record.rdata : [];
						instance.ttl = record.ttl;
					}
					else {
						instance.port = record.rdata.port;
						instance.target = record.rdata.target.join(".");
						instance.ttl = record.ttl;
					}
					instance.changed = changed = true;
					break;
			}
		}
		if (!changed)
			return;

		this.monitors.forEach(monitor => {
			monitor.forEach(instance => {
				if (!instance.changed)
					return;
				delete instance.changed;
				if (instance.name && instance.txt && instance.target && instance.address)
					monitor.callback.call(this, monitor.service.slice(0, -6), instance);
				else {
					let query = new Serializer({query: true, opcode: DNS.OPCODE.QUERY});

					if (instance.target)
						query.add(DNS.SECTION.QUESTION, instance.target, DNS.RR.A, DNS.CLASS.IN | MDNS.UNICAST);
					query.add(DNS.SECTION.QUESTION, instance.name + "." + monitor.service, DNS.RR.SRV, DNS.CLASS.IN | MDNS.UNICAST);
					query.add(DNS.SECTION.QUESTION, instance.name + "." + monitor.service, DNS.RR.TXT, DNS.CLASS.IN | MDNS.UNICAST);

					this.write(MDNS.IP, MDNS.PORT, query.build());
				}
			});
		});
	}

	callback(message, value, address, port) {
		const packet = new Parser(this.read(ArrayBuffer))
		let response = new Array(2);

//		dumpPacket(packet, address);

		if (packet.answers || packet.additionals)
			this.scanPacket(packet, address);

		if (this.probing) {
			if (this.probing < 0) return;

			for (let i = 0; i < packet.questions; i++) {
				const question = packet.question(i);
				if (DNS.RR.ANY !== question.qtype)
					continue;
				if ((this.hostName !== question.qname[0]) || (2 !== question.qname.length) || (LOCAL !== question.qname[1]))
					continue;

				trace(`probe tie-break with ${address}\n`);
				for (let j = 0; j < packet.authorities; j++) {
					const authority = packet.authority(j);
					if (DNS.RR.A !== authority.qtype)
						continue;
					if ((this.hostName !== authority.qname[0]) || (2 !== authority.qname.length) || (LOCAL !== authority.qname[1]))
						continue;
					const rdata = authority.rdata.split(".");
					const ip = Net.get("IP").split(".");
					trace(`  compare ${rdata} with ${ip}\n`);
					for (let k = 0; k < 4; k++) {
						if (parseInt(ip[k]) < parseInt(rdata[k])) {
							trace(`  we lose\n`);
							this.probing = -1000;	// try again in one second
							return;
						}
					}
					trace(`  we win\n`);		// we are lexicographically same or later, so can ignore
				}
			}

			return;
		}

		if (packet.flags & 0x8000)
			return;

		for (let h = 0, length = this.services.length ? this.services.length : 1; h < length; h++) {	// always at least one pass, to pick up service-indepdent qtypes
			const service = this.services[h];
			let mask = [0, 0];

			for (let i = 0; i < packet.questions; i++) {
				const question = packet.question(i);
				const name = question.qname;

				switch (question.qtype) {
					case DNS.RR.A:
						if (h)
							;
						else if ((2 === name.length) && (LOCAL === name[1]) && (this.hostName == name[0]))
							mask[question.qclass >> 15] |= 0x01;
						break;

					case DNS.RR.AAAA:
						if (!h && (2 === name.length) && (LOCAL === name[1]) && (this.hostName == name[0]))
							mask[question.qclass >> 15] |= 0x20;
						break;

					case DNS.RR.PTR:
						if (!service)
							;
						else
						if ((3 === name.length) && (LOCAL === name[2])) {
							// service query
							if ((("_" + service.name) === name[0]) && (("_" + service.protocol) === name[1])) {
								let respond = true;

								for (let j = 0; (j < packet.answers) && respond; j++) {
									let answer = packet.answer(j);
									if (DNS.RR.PTR !== answer.qtype)
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
									mask[question.qclass >> 15] |= 0x1F;
							}
						}
						else
						if ((4 === name.length) && ("_services" === name[0]) && ("_dns-sd" === name[1]) && ("_udp" === name[2]) && (LOCAL === name[3])) {
							let respond = true;
							for (let j = 0; (j < packet.answers) && respond; j++) {
								let answer = packet.answer(j);
								if (DNS.RR.PTR !== answer.qtype)
									continue;

								if ((3 === answer.rdata.length) && (("_" + service.name) === answer.rdata[0]) &&
								   (("_" + service.protocol) === answer.rdata[1]) && (LOCAL === answer.rdata[2]))
								   respond = false;
							}
							if (respond)
								mask[question.qclass >> 15] |= 0x10;
							break;
						}
						break;

					case DNS.RR.SRV:
						if (!service)
							;
						else
						if ((4 === name.length) && (LOCAL === name[3]) && (this.hostName === name[0]) &&
							(("_" + service.name) === name[1]) && (("_" + service.protocol) === name[2]))
							mask[question.qclass >> 15] |= 0x02;		// not checking known answers... seems to never be populated.
						break;

					case DNS.RR.ANY:
						if ((2 === name.length) && (LOCAL === name[1]) && (this.hostName === name[0]))
							mask[question.qclass >> 15] |= service ? 0x1F : 0x01;
						break;
				}
			}

			if (mask[0]) {
				if (!response[0])
					response[0] = new Serializer({query: false, opcode: DNS.OPCODE.QUERY, authoritative: true});
				this.reply(response[0], mask[0], service)
			}
			if (mask[1]) {
				if (!response[1])
					response[1] = new Serializer({query: false, opcode: DNS.OPCODE.QUERY, authoritative: true});
				this.reply(response[1], mask[1], service)
			}
		}

		//@@ delay response 20-120 ms
		if (response[0])
			this.write(MDNS.IP, MDNS.PORT, response[0].build(packet.id));
		if (response[1])
			this.write(address, port, response[1].build(packet.id));
	}

	/*

		0x8000 - CACHE FLUSH TEXT
		0x20 - NSEC - A ONLY
		0x10 - dns-sd service PTR
		0x08 - PTR
		0x04 - TXT
		0x02 - SRV
		0x01 - A
	 */
	reply(response, mask, service, bye = false) {
		let build = response ? false : true;
		if (build) response = new Serializer({query: false, opcode: DNS.OPCODE.QUERY, authoritative: true});
		bye = bye ? 0 : 0xFFFFFFF;

		if (0x20 & mask)	// NSEC indicating A only
			response.add(DNS.SECTION.ANSWER, this.hostName + "." + LOCAL, DNS.RR.NSEC, DNS.CLASS.IN | MDNS.FLUSH, TTL & bye,
						 {next: this.hostName + "." + LOCAL, bitmaps: Uint8Array.of(0, 4, 0x40, 0, 0, 0)});

		if (0x10 & mask)	// PTR for service discovery
			response.add(DNS.SECTION.ANSWER, "_services._dns-sd._udp.local", DNS.RR.PTR, DNS.CLASS.IN, TTL & bye, "_" + service.name + "._" + service.protocol + "." + LOCAL);

		if (0x08 & mask)		// PTR
			response.add(DNS.SECTION.ANSWER, "_" + service.name + "._" + service.protocol + "." + LOCAL, DNS.RR.PTR, DNS.CLASS.IN, TTL & bye, this.hostName + "._" + service.name + "._" + service.protocol + "." + LOCAL);

		if (0x04 & mask)		// TXT
			response.add(DNS.SECTION.ANSWER, this.hostName + "._" + service.name + "._" + service.protocol + "." + LOCAL, DNS.RR.TXT, DNS.CLASS.IN | ((0x8000 & mask) ? MDNS.FLUSH : 0), TTL & bye, service.txt);

		if (0x02 & mask)	// SRV
			response.add(DNS.SECTION.ANSWER, this.hostName + "._" + service.name + "._" + service.protocol + "." + LOCAL, DNS.RR.SRV, DNS.CLASS.IN, 120 & bye,
							{priority: 0, weight: 0, port: service.port, target: this.hostName + "." + LOCAL});

		if (0x01 & mask)	// A
			response.add(DNS.SECTION.ANSWER, this.hostName + "." + LOCAL, DNS.RR.A, DNS.CLASS.IN | MDNS.FLUSH, 120 & bye, Net.get("IP"));

		if (!build)
			return;

		return response.build();
	}

	/*
		 Probe
			 short delay of 0 to 250 ms.
			 send DNS.RR.ANY at 0, 250, and 500 ms
				 the probes SHOULD be sent as "QU" questions with the unicast-response bit set
				 each host populates the query message's Authority Section with the record that it would be proposing to use, should its probing be successful
			 if no conflict after 750 ms, we own the name
			 if conflict in that time,
				 choose new name
				 start probe again
			 if tie...
	*/
	probe() {
		this.probing = 1;
		this.probeAttempt = 1;
		trace(`probe for ${this.hostName}\n`);
		this.client(1, "");
		Timer.repeat(id => {
			if (this.probing < 0) {
				 let hostName = this.client(2, this.hostName);
				 if (hostName) {
					 if ("string" == typeof hostName) {
						 this.hostName = hostName;
						 this.probeAttempt = 1;
					 }
					 else {
						Timer.clear(id);
//						delete this.probing;
						delete this.probeAttempt;
						delete this.hostName;	// no hostName claimed, no longer probing
						this.client(-1);
						return;
					 }
				 }
				 else {
					 if (this.probeAttempt > 2) {
						 hostName = this.hostName.split("-");
						 hostName.pop();
						 this.hostName = hostName.join("-");
					 }
					 if (this.probeAttempt >= 2)
						 this.hostName += "-" + this.probeAttempt;
				}
				Timer.schedule(id, -this.probing, 250);
				this.probing = 1;
				trace(`re-probe for ${this.hostName}\n`);
			}
			if (4 === this.probing) {
				trace(`probe claimed ${this.hostName}\n`);

				Timer.clear(id);
				delete this.probing;
				delete this.probeAttempt;

				this.client(1, this.hostName);

				return;
			}

			trace(`probe ${this.probing}\n`);
			const reply = new Serializer({query: true, opcode: DNS.OPCODE.QUERY, authoritative: true});
			reply.add(DNS.SECTION.QUESTION, this.hostName + "." + LOCAL, DNS.RR.ANY, DNS.CLASS.IN | MDNS.UNICAST);	// question for DNS.RR.A, unicast response requested
			reply.add(DNS.SECTION.ANSWER, this.hostName + "." + LOCAL, DNS.RR.A, DNS.CLASS.IN | MDNS.FLUSH, 120, Net.get("IP"));	// authoritative answer for DNS.RR.A
			this.write(MDNS.IP, MDNS.PORT, reply.build());
			this.probing += 1;
		}, 250, 10);		//@@ random initial delay
	}
}
MDNS.IP = "224.0.0.251";
MDNS.PORT = 5353;
MDNS.FLUSH = 0x8000;
MDNS.UNICAST = 0x8000;

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

export default MDNS;
