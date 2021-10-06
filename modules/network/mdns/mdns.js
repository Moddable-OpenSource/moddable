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
			this.hostNameLower = this.hostName.toLowerCase();
			this.probing = 1;
			Timer.set(() => this.probe(), 0);
		}
	}
	close() {
		while (this.services.length)
			this.remove(this.services[0]);

		while (this.monitors.length)
			this.remove(this.monitors[0].service.substring(0, this.monitors[0].service.length - 6));

		if (this.probeTimer)
			Timer.clear(this.probeTimer);

		super.close();
	}
	add(service) {
		if (!this.hostName)
			throw new Error("no hostName");

		if (this.probing)
			throw new Error("not ready");

		if (!service.txt)
			service.txt = {};
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
		}, 500);
		service.timer.interval = 500;
	}
	update(service) {
		const index = this.services.indexOf(service);
		if (index < 0) throw new Error("service not found");

		if (!service.updater) {
			service.updater = Timer.set(updater => {
				this.write(MDNS.IP, MDNS.PORT, this.reply(null, 0x04 | 0x8000, service));
				updater.interval += 250;
				if (updater.interval >= 1500) {
					Timer.clear(updater);
					delete service.updater;
					delete service.update;
				}
				else
					Timer.schedule(updater, updater.interval, 250);
			}, 0, 250);
		}
		else
			Timer.schedule(service.updater, 0, 250);
		service.updater.interval = 0;
	}
	remove(service, callback) {
		if ("string" === typeof service) {
			service += ".local";
			service = this.monitors.findIndex(item => item.service === service);
			if (service >= 0) {
				callback = this.monitors[service].callbacks.findIndex(item => item === callback);
				if (callback < 0) throw new Error("callback not found");
				this.monitors[service].callbacks.splice(callback, 1);
				if (0 === this.monitors[service].callbacks.length) {
					this.monitors[service].close = true;
					Timer.clear(this.monitors[service].timer);
					this.monitors.splice(service, 1);
				}
			}
		}
		else {
			const index = this.services.indexOf(service);
			if (index < 0) throw new Error("service not found");

			this.services.splice(index, 1);
			try {
				this.write(MDNS.IP, MDNS.PORT, this.reply(null, 0x0F, service, true));
			}
			catch {
				// write fails if socket closed
			}

			if (service.timer) {
				Timer.clear(service.timer);
				delete service.timer;
			}
		}
	}
	monitor(service, callback) {
		service += ".local";
		let monitor = this.monitors.find(item => item.service === service);
		if (monitor) {
			if (monitor.callbacks.find(item => item === callback))
				throw new Error("redundant");
			monitor.callbacks.push(callback);
			if (!monitor.length)
				return;

			Timer.set(() => {
				for (let i = 0; i < monitor.length; i++)
					callback.call(this, monitor.service.slice(0, -6), monitor[i]);
			}, 0);
		}
		else {
			monitor = [];
			monitor.service = service;
			monitor.state = 0;
			monitor.callbacks = [callback];
			monitor.mdns = this;
			monitor.timer = Timer.set(this.monitorTask.bind(monitor), 1);
			this.monitors.push(monitor);
		}
	}
	monitorTask() {
		const mdns = this.mdns;
		switch (this.state) {
			case 0:
			case 1:
			case 2:
			case 3:
				const query = new Serializer({query: true, opcode: DNS.OPCODE.QUERY});

				query.add(DNS.SECTION.QUESTION, this.service, DNS.RR.PTR, DNS.CLASS.IN | ((0 == this.state) ? MDNS.UNICAST : 0));

				for (let i = 0; i < this.length; i++)
					query.add(DNS.SECTION.ANSWER, this.service, DNS.RR.PTR, DNS.CLASS.IN, TTL, this[i].name + "." + this.service);

				try {
					mdns.write(MDNS.IP, MDNS.PORT, query.build());
				}
				catch {
					this.timer = Timer.set(mdns.monitorTask.bind(this), 10 * 1000);
					return;
				}

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
		let changed, i, j, length;
		for (i = 0, length = answers + packet.additionals; i < length; i++) {
			const record = (i < answers) ? packet.answer(i) : packet.additional(i - answers);
			let name = record.qname, service;
			let monitor, instance;

			switch (record.qtype) {
				case DNS.RR.A:
					//@@ if not probing, check to see if name matches and IP is different. If so there is a conflict, revert to probing state...
					if ((this.probing > 0) && (this.hostNameLower == name[0].toLowerCase()) && (2 === name.length) && (LOCAL === name[1].toLowerCase())) {
						trace(`probe conflict with ${address}\n`);
						this.probing = -1;
						this.probeAttempt += 1;
						break;
					}
					name = name.join(".");
					this.monitors.forEach(monitor => {
						monitor.forEach(instance => {
							if (instance.target?.toLowerCase() === name.toLowerCase())
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
					instance = monitor.find(item => item.name.toLowerCase() === name[0].toLowerCase());
					if (!instance) {
						instance = {name: name[0]};
						monitor.push(instance);
					}
					if (DNS.RR.TXT === record.qtype) {
						const txt = record.rdata ? record.rdata : [];
						if (!instance.txt || (instance.txt.length !== txt.length)) {
							instance.txt = txt;
							instance.changed = changed = true;
						}
						else {
							for (j = 0; j < txt.length; ++j) {
								if (txt[j] === instance.txt[j])
									continue;
								instance.txt = txt;
								instance.changed = changed = true;
								break;
							}
						}
					}
					else {
						const target = record.rdata.target.join(".");
						if ((instance.port !== record.rdata.port) || (instance.target.toLowerCase() !== target.toLowerCase())) {
							instance.port = record.rdata.port;
							instance.target = target;
							instance.changed = changed = true;
						}
					}
					instance.ttl = record.ttl;
					break;
			}
		}
		if (!changed)
			return;

		//@@ could fire a callback to do this scan... might be better
		const monitors = Array.from(this.monitors);
		for (i = 0; i < monitors.length; i++) {
			const monitor = monitors[i];
			for (j = 0; (j < monitor.length) && !monitor.closed; j++) {
				const instance = monitor[j];
				if (!instance.changed)
					continue;
				delete instance.changed;
				if (instance.name && instance.txt && instance.target && instance.address) {
					const callbacks = Array.from(monitor.callbacks);
					for (let k = 0; k < callbacks.length; k++) {
						const callback = callbacks[k];

						if (monitor.callbacks.indexOf(callback) < 0)		// expensive check to see if callback was closed
							continue;

						try {
							callback.call(this, monitor.service.slice(0, -6), instance);
						}
						catch {
						}
					}
				}
				else {
					let query = new Serializer({query: true, opcode: DNS.OPCODE.QUERY});

					if (instance.target)
						query.add(DNS.SECTION.QUESTION, instance.target, DNS.RR.A, DNS.CLASS.IN | MDNS.UNICAST);
					query.add(DNS.SECTION.QUESTION, instance.name + "." + monitor.service, DNS.RR.SRV, DNS.CLASS.IN | MDNS.UNICAST);
					query.add(DNS.SECTION.QUESTION, instance.name + "." + monitor.service, DNS.RR.TXT, DNS.CLASS.IN | MDNS.UNICAST);

					this.write(MDNS.IP, MDNS.PORT, query.build());
				}
			}
		}
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
				if ((this.hostNameLower !== question.qname[0].toLowerCase()) || (2 !== question.qname.length) || (LOCAL !== question.qname[1].toLowerCase()))
					continue;

				trace(`probe tie-break with ${address}\n`);
				for (let j = 0; j < packet.authorities; j++) {
					const authority = packet.authority(j);
					if (DNS.RR.A !== authority.qtype)
						continue;
					if ((this.hostNameLower !== authority.qname[0].toLowerCase()) || (2 !== authority.qname.length) || (LOCAL !== authority.qname[1].toLowerCase()))
						continue;
					const rdata = authority.rdata.split(".");
					const ip = Net.get("IP", address).split(".");
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
				for (let j = 0; j < name.length; j++)
					name[j] = name[j].toLowerCase();

				switch (question.qtype) {
					case DNS.RR.A:
						if (h)
							;
						else if ((2 === name.length) && (LOCAL === name[1]) && (this.hostNameLower == name[0]))
							mask[question.qclass >> 15] |= 0x01;
						break;

					case DNS.RR.AAAA:
						if (!h && (2 === name.length) && (LOCAL === name[1]) && (this.hostNameLower == name[0]))
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
								   if ((3 !== answer.qname.length) || (("_" + service.name) !== answer.qname[0].toLowerCase()) ||
									   (("_" + service.protocol) !== answer.qname[1].toLowerCase()) || (LOCAL !== answer.qname[2].toLowerCase()))
									   continue;
								   if ((4 !== answer.rdata.length) || (this.hostNameLower !== answer.rdata[0].toLowerCase()) || (("_" + service.name) !== answer.rdata[1].toLowerCase()) ||
									   (("_" + service.protocol) !== answer.rdata[2].toLowerCase()) || (LOCAL !== answer.rdata[3].toLowerCase()))
									   continue;
								   respond = false;
								}
								if (respond)
									mask[question.qclass >> 15] |= 0x1F;
							}
						}
						else
						if ((4 === name.length) && ("_services" === name[0].toLowerCase()) && ("_dns-sd" === name[1].toLowerCase()) && ("_udp" === name[2].toLowerCase()) && (LOCAL === name[3].toLowerCase())) {
							let respond = true;
							for (let j = 0; (j < packet.answers) && respond; j++) {
								let answer = packet.answer(j);
								if (DNS.RR.PTR !== answer.qtype)
									continue;

								if ((3 === answer.rdata.length) && (("_" + service.name.toLowerCase()) === answer.rdata[0].toLowerCase()) &&
								   (("_" + service.protocol) === answer.rdata[1].toLowerCase()) && (LOCAL === answer.rdata[2].toLowerCase()))
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
						if ((4 === name.length) && (LOCAL === name[3].toLowerCase()) && (this.hostNameLower === name[0].toLowerCase()) &&
							(("_" + service.name.toLowerCase()) === name[1].toLowerCase()) && (("_" + service.protocol.toLowerCase()) === name[2].toLowerCase()))
							mask[question.qclass >> 15] |= 0x02;		// not checking known answers... seems to never be populated.
						break;

					case DNS.RR.ANY:
						if ((2 === name.length) && (LOCAL === name[1].toLowerCase()) && (this.hostNameLower === name[0].toLowerCase()))
							mask[question.qclass >> 15] |= service ? 0x1F : 0x01;
						break;
				}
			}

			if (mask[0]) {
				if (!response[0])
					response[0] = new Serializer({query: false, opcode: DNS.OPCODE.QUERY, authoritative: true});
				this.reply(response[0], mask[0], service, false, address)
			}
			if (mask[1]) {
				if (!response[1])
					response[1] = new Serializer({query: false, opcode: DNS.OPCODE.QUERY, authoritative: true});
				this.reply(response[1], mask[1], service, false, address)
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
	reply(response, mask, service, bye = false, from) {
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
			response.add(DNS.SECTION.ANSWER, this.hostName + "." + LOCAL, DNS.RR.A, DNS.CLASS.IN | MDNS.FLUSH, 120 & bye, Net.get("IP", from));

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
		this.client(MDNS.hostName, "");
		this.probeTimer = Timer.repeat(id => {
			if (this.probing < 0) {
				 let hostName = this.client(MDNS.retry, this.hostName);
				 if (hostName) {
					 if ("string" == typeof hostName) {
						 this.hostName = hostName;
						 this.hostNameLower = hostName.toLowerCase() 
						 this.probeAttempt = 1;
					 }
					 else {
						Timer.clear(id);
						delete this.probeTimer;
//						delete this.probing;
						delete this.probeAttempt;
						delete this.hostName;	// no hostName claimed, no longer probing
						delete this.hostNameLower;
						this.client(MDNS.error);
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
					this.hostNameLower = this.hostName.toLowerCase();
				}
				Timer.schedule(id, -this.probing, 250);
				this.probing = 1;
				trace(`re-probe for ${this.hostName}\n`);
			}
			if (4 === this.probing) {
				trace(`probe claimed ${this.hostName}\n`);

				Timer.clear(id);
				delete this.probeTimer;
				delete this.probing;
				delete this.probeAttempt;

				this.client(MDNS.hostName, this.hostName);

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

MDNS.hostName = 1;
MDNS.retry = 2;
MDNS.error = -1;

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

	for (let i = 0; i < packet.additionals; i++) {
		let a = packet.additional(i);
		trace("  +: ");
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
