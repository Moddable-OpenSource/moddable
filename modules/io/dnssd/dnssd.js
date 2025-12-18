/*
 * Copyright (c) 2018-2025 Moddable Tech, Inc.
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
		unicast reponse only to addresses on same subnet
*/

import Parser from "dns/parser";
import Serializer from "dns/serializer";
import DNS from "dns";
import Net from "net";
import Timer from "timer";

const LOCAL = "local";
const TTL = 4500;

class DNSSD {
	udp;
	services = [];
	monitors = [];
	claims = [];

	constructor(options) {
		this.udp = new (options.socket.io)({
			...options.socket,
			target: this,
			port: DNSSD.PORT,
			onReadable(packets) {
				do {
					const packet = this.read();
					this.target.#onPacket(packet, packet.address, packet.port);
				} while (--packets);
			}
		});
		this.udp.add(DNSSD.IP);
	}
	close() {
		while (this.services.length)
			this.remove(this.services[0]);

		while (this.monitors.length)
			this.remove(this.monitors[0].service.substring(0, this.monitors[0].service.length - 6));

		this.claims.forEach(claim => {
			Timer.clear(claim.timer);
			Timer.clear(claim.initial);
		});
		delete this.claims;
		
		this.udp?.close();
		delete this.udp;
	}
	write(buffer, address = DNSSD.IP, port = DNSSD.PORT) {
		try { 
			this.udp.write(buffer, address, port);
		}
		catch {
			return false;
		}
		return true;
	}
	claim(options) {
		if (this.claims.some(claim => claim.host === options.host))
			throw new Error("duplicate");

		const claim = {
			host: options.host,
			hostLower: options.host.toLowerCase(),
			onReady: options.onReady,
			onError: options.onError,
			probing: 1,
			initial: Timer.set(() => this.probe(claim), 0),
			probeAttempt: 1,
		};
		this.claims.push(claim);
		return claim;
	}
	unclaim(claim) {
		const i = this.claims.indexOf(claim);
		if (i < 0)
			return;

		this.claims.splice(i, 1);

		Timer.clear(claim.timer);
		delete claim.timer;

		Timer.clear(claim.initial);
		delete claim.initial;
	}
	add(service) {
		this.services.push(service);

		this.write(this.reply(null, 0x1F | 0x8000, service, true));		// remove before adding to clear stale state in clients
		service.timer = Timer.repeat(timer => {
			this.write(this.reply(null, 0x1F, service));
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
				this.write(this.reply(null, 0x04 | 0x8000, service));
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
			this.write(this.reply(null, 0x0F, service, true));

			Timer.clear(service.timer);
			delete service.timer;
			Timer.clear(service.updater);
			delete service.updater;
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
			monitor.dnssd = this;
			monitor.timer = Timer.set(this.monitorTask.bind(monitor), 1);
			this.monitors.push(monitor);
		}
	}
	monitorTask() {
		const dnssd = this.dnssd;
		if ((this.state < 0) || (this.state >= 4))
			return;

		const query = new Serializer({query: true, opcode: DNS.OPCODE.QUERY});

		query.add(DNS.SECTION.QUESTION, this.service, DNS.RR.PTR, DNS.CLASS.IN | ((0 == this.state) ? DNSSD.UNICAST : 0));

		for (let i = 0; i < this.length; i++)
			query.add(DNS.SECTION.ANSWER, this.service, DNS.RR.PTR, DNS.CLASS.IN, TTL, this[i].name + "." + this.service);

		if (!dnssd.write(query.build())) {
			this.timer = Timer.set(dnssd.monitorTask.bind(this), 10 * 1000);
			return;
		}

		if (this.state < 3) {
			this.timer = Timer.set(dnssd.monitorTask.bind(this), (2 ** this.state) * 1000);
			this.state += 1;
		}
		else
			this.timer = Timer.set(dnssd.monitorTask.bind(this), 30 * 60 * 1000);		//@@ calculate based on TTL elapsed (section 5.2 Continuous Multicast DNS Querying)
	}
	scanPacket(packet) {
		const answers = packet.answers, records = [];
		for (let i = 0, length = answers + packet.additionals; i < length; i++)
			records.push((i < answers) ? packet.answer(i) : packet.additional(i - answers));

		let changed;
		records.forEach(record => {
			if ((DNS.RR.SRV !== record.qtype) && (DNS.RR.TXT !== record.qtype)) return;

			const name = record.qname;
			const service = name.slice(1).join(".");
			const monitor = this.monitors.find(monitor => monitor.service === service);
			if (!monitor)
				return;
			let instance = monitor.find(item => item.name.toLowerCase() === name[0].toLowerCase());
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
					for (let j = 0; j < txt.length; ++j) {
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
		});

		records.forEach(record => {
			if (DNS.RR.PTR !== record.qtype) return;

			const service = record.qname.join(".");
			const monitor = this.monitors.find(monitor => monitor.service === service);
			if (!monitor)
				return;
			let instance = monitor.find(item => item.name === record.rdata[0]);
			if (!instance) {
				instance = {name: record.rdata[0]};
				monitor.push(instance);
				instance.changed = changed = true;
			}
			instance.ttl = record.ttl
		});

		records.forEach(record => {
			if (DNS.RR.A !== record.qtype) return;

			let name = record.qname;
			//@@ if not probing, check to see if name matches and IP is different. If so there is a conflict, error out the claim...
			this.claims.forEach(claim => {
				if ((claim.probing > 0) && (claim.hostLower == name[0].toLowerCase()) && (2 === name.length) && (LOCAL === name[1].toLowerCase())) {
					claim.probing = -1;
					claim.probeAttempt += 1;
				}
			});

			name = name.join(".").toLowerCase();
			this.monitors.forEach(monitor => {
				monitor.forEach(instance => {
					if (instance.target?.toLowerCase() === name)
						instance.address = record.rdata;
				});
			});
		});

		if (!changed)
			return;

		const monitors = Array.from(this.monitors);
		for (let i = 0; i < monitors.length; i++) {
			const monitor = monitors[i];
			for (let j = 0; (j < monitor.length) && !monitor.closed; j++) {
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
							/* this space intentionally left blank */
						}
					}
				}
				else {
					let query = new Serializer({query: true, opcode: DNS.OPCODE.QUERY});

					if (instance.target)
						query.add(DNS.SECTION.QUESTION, instance.target, DNS.RR.A, DNS.CLASS.IN | DNSSD.UNICAST);
					query.add(DNS.SECTION.QUESTION, instance.name + "." + monitor.service, DNS.RR.SRV, DNS.CLASS.IN | DNSSD.UNICAST);
					query.add(DNS.SECTION.QUESTION, instance.name + "." + monitor.service, DNS.RR.TXT, DNS.CLASS.IN | DNSSD.UNICAST);

					this.write(query.build());
				}
			}
		}
	}

	#onPacket(packet, address, port) {
		packet = new Parser(packet)
		let response = new Array(2);

//		dumpPacket(packet, address);

		if (packet.answers || packet.additionals)
			this.scanPacket(packet);

		this.claims.forEach(claim => {
			if (claim.probing < 0) return;

			for (let i = 0; i < packet.questions; i++) {
				const question = packet.question(i);
				if (DNS.RR.ANY !== question.qtype)
					continue;
				if ((claim.hostLower !== question.qname[0].toLowerCase()) || (2 !== question.qname.length) || (LOCAL !== question.qname[1].toLowerCase()))
					continue;

				trace(`probe tie-break with ${address}\n`);
				for (let j = 0; j < packet.authorities; j++) {
					const authority = packet.authority(j);
					if (DNS.RR.A !== authority.qtype)
						continue;
					if ((claim.hostLower !== authority.qname[0].toLowerCase()) || (2 !== authority.qname.length) || (LOCAL !== authority.qname[1].toLowerCase()))
						continue;
					const rdata = authority.rdata.split(".");
					const ip = Net.get("IP", address).split(".");
					trace(`  compare ${rdata} with ${ip}\n`);
					for (let k = 0; k < 4; k++) {
						if (parseInt(ip[k]) < parseInt(rdata[k])) {
							trace(`  we lose\n`);
							claim.probing = -1000;	// try again in one second
							return;
						}
					}
					trace(`  we win\n`);		// we are lexicographically same or later, so can ignore
				}
			}
		});

		if (packet.flags & 0x8000)
			return;

		for (let h = 0, length = this.services.length ? this.services.length : 1; h < length; h++) {	// always at least one pass, to pick up service-independent qtypes
			const service = this.services[h];
			let mask = [0, 0];

			for (let i = 0; i < packet.questions; i++) {
				const question = packet.question(i);
				const name = question.qname;
				for (let j = 0; j < name.length; j++)
					name[j] = name[j].toLowerCase();

				switch (question.qtype) {
					case DNS.RR.ANY:
						if (service) {
							if ((2 === name.length) && (LOCAL === name[1].toLowerCase()) && (service.hostLower === name[0].toLowerCase()))
								mask[question.qclass >> 15] |= 0x1F;
							break;
						}
						// fallthrough to DNS.RR.A

					case DNS.RR.A:
					case DNS.RR.AAAA:
						if (!h && (2 === name.length) && (LOCAL === name[1])) {
							this.claims.some(claim => {
								if (claim.hostLower === name[0]) {
									const index = question.qclass >> 15;
									response[index] ??= new Serializer({query: false, opcode: DNS.OPCODE.QUERY, authoritative: true});
									this.reply(response[index], (DNS.RR.AAAA === question.qtype) ? 0x20 : 0x01, null, false, address, claim.hostLower);
									return true;
								}
							});
						}
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
								   if ((4 !== answer.rdata.length) || (service.hostLower !== answer.rdata[0].toLowerCase()) || (("_" + service.name) !== answer.rdata[1].toLowerCase()) ||
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
						if ((4 === name.length) && (LOCAL === name[3].toLowerCase()) && (service.hostLower === name[0].toLowerCase()) &&
							(("_" + service.name.toLowerCase()) === name[1].toLowerCase()) && (("_" + service.protocol.toLowerCase()) === name[2].toLowerCase()))
							mask[question.qclass >> 15] |= 0x02;		// not checking known answers... seems to never be populated.
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
			this.write(response[0].build(packet.id));
		if (response[1])
			this.write(response[1].build(packet.id), address, port);
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
	reply(response, mask, service, bye = false, from, host = service.host) {
		const build = response ? false : true;
		if (build) response = new Serializer({query: false, opcode: DNS.OPCODE.QUERY, authoritative: true});
		bye = bye ? 0 : 0xFFFFFFF;
		const instanceName = service?.instanceName ?? host;

		if (0x20 & mask)	// NSEC indicating A only
			response.add(DNS.SECTION.ANSWER, host + "." + LOCAL, DNS.RR.NSEC, DNS.CLASS.IN | DNSSD.FLUSH, TTL & bye,
						 {next: host + "." + LOCAL, bitmaps: Uint8Array.of(0, 4, 0x40, 0, 0, 0)});

		if (0x10 & mask)	// PTR for service discovery
			response.add(DNS.SECTION.ANSWER, "_services._dns-sd._udp.local", DNS.RR.PTR, DNS.CLASS.IN, TTL & bye, "_" + service.name + "._" + service.protocol + "." + LOCAL);

		if (0x08 & mask)		// PTR
			response.add(DNS.SECTION.ANSWER, "_" + service.name + "._" + service.protocol + "." + LOCAL, DNS.RR.PTR, DNS.CLASS.IN, TTL & bye, instanceName + "._" + service.name + "._" + service.protocol + "." + LOCAL);

		if ((0x04 & mask) && service.txt)		// TXT
			response.add(DNS.SECTION.ANSWER, instanceName + "._" + service.name + "._" + service.protocol + "." + LOCAL, DNS.RR.TXT, DNS.CLASS.IN | ((0x8000 & mask) ? DNSSD.FLUSH : 0), TTL & bye, service.txt);

		if (0x02 & mask)	// SRV
			response.add(DNS.SECTION.ANSWER, instanceName + "._" + service.name + "._" + service.protocol + "." + LOCAL, DNS.RR.SRV, DNS.CLASS.IN, 120 & bye,
							{priority: 0, weight: 0, port: service.port, target: host + "." + LOCAL});

		if (0x01 & mask)	// A
			response.add(DNS.SECTION.ANSWER, host + "." + LOCAL, DNS.RR.A, DNS.CLASS.IN | DNSSD.FLUSH, 120 & bye, Net.get("IP", from));

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
	probe(claim) {
		delete claim.initial;
		trace(`probe for ${claim.host}\n`);
		claim.timer = Timer.repeat(id => {
			if (claim.probing < 0) {
				claim?.onError();
				this.unclaim(claim);
				return;
			}

			if (4 === claim.probing) {
				trace(`probe claimed ${claim.host}\n`);

				Timer.clear(id);
				delete claim.timer;
				delete claim.probing;
				delete claim.probeAttempt;

				claim.onReady?.();
				return;
			}

			trace(`probe ${claim.probing}\n`);
			const reply = new Serializer({query: true, opcode: DNS.OPCODE.QUERY, authoritative: true});
			reply.add(DNS.SECTION.QUESTION, claim.host + "." + LOCAL, DNS.RR.ANY, DNS.CLASS.IN | DNSSD.UNICAST);	// question for DNS.RR.A, unicast response requested
			reply.add(DNS.SECTION.ANSWER, claim.host + "." + LOCAL, DNS.RR.A, DNS.CLASS.IN | DNSSD.FLUSH, 120, Net.get("IP"));	// authoritative answer for DNS.RR.A
			this.write(reply.build());
			claim.probing += 1;
		}, 250, 10);		//@@ random initial delay
	}
}
DNSSD.IP = "224.0.0.251";
DNSSD.PORT = 5353;
DNSSD.FLUSH = 0x8000;
DNSSD.UNICAST = 0x8000;

DNSSD.hostName = 1;
DNSSD.retry = 2;
DNSSD.error = -1;

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

class Advertise {
	#service;
	#list;

	constructor(options, list) {
		const lastIndexOf = options.serviceType.lastIndexOf("._");
		if (lastIndexOf < 0)
			throw new Error("invalid serviceType");

		const {txt, name, serviceType} = options;
		this.#service = {
			name: serviceType.slice(1, lastIndexOf),
			protocol: serviceType.slice(lastIndexOf + 2),
			host: options.host,
			port: options.port
		};
		if (txt)
			this.#service.txt = txt;
		if (name)
			this.#service.instanceName = name;
		dnssd.add(this.#service);
		
		this.#list = list;
		list.push(this);
	}
	close() {
		if (!this.#service) return
		dnssd.remove(this.#service);
		this.#service = undefined;
		this.#list.splice(this.#list.indexOf(this), 1);
	}
	updateTXT(txt) {
		if (txt)
			this.#service.txt = txt;
		else
			delete this.#service.txt;
		dnssd.update(this.#service);
	}
}

class Discover {
	#serviceType;
	#callback;
	#list;
	#onFound;
	#onUpdate;
//@@	#onLost;
	
	constructor(options, list) {
		this.#onFound = options.onFound;
		this.#onUpdate = options.onUpdate;
//@@		this.#onLost = options.onLost;
		this.#serviceType = options.serviceType;

		this.#callback = (service, instance) => {
			const found = {
				name: instance.name,
				host: instance.target,
				address: instance.address,
				port: instance.port
			};
			if (instance.txt?.length) {
				found.txt = new Map;
				for (let i = 0, txt = instance.txt, length = txt.length; i < length; i++) {
					const equal = txt[i].indexOf("=");
					if (equal < 0)
						found.txt.set(txt[i], undefined);
					else
						found.txt.set(txt[i].slice(0, equal), txt[i].slice(equal + 1));
				}
			}
			if (instance.found)
				this.#onUpdate?.(found);
			else {
				instance.found = true;
				this.#onFound?.(found);
			}
		};
		dnssd.monitor(options.serviceType, this.#callback);

		this.#list = list;
		list.push(this);
	}
	close() {
		if (!this.#serviceType) return
		dnssd.remove(this.#serviceType, this.#callback);
		this.#list.splice(this.#list.indexOf(this), 1);
		this.#serviceType = undefined;
	}
}

//@@ ttl
class Claim  {
	#claim;
	#list;
	#onReady;
	#onError;

	constructor(options, list) {
		this.#onReady = options.onReady;
		this.#onError = options.onError;
		this.#claim = dnssd.claim({
			...options,
			onReady: () => this.#onReady?.(),
			onError: () => this.#onError?.(),
		});
		this.#list = list;
		list.push(this);
	}
	close() {
		if (!this.#claim) return;
		dnssd.unclaim(this.#claim);
		this.#list.splice(this.#list.indexOf(this), 1);
		this.#claim = undefined;
	}
}

let dnssd;

export default class {
	#list = [];

	constructor(options) {
		dnssd ??= new DNSSD(options);
		dnssd.count ??= 0;
		dnssd.count += 1;
	}
	close() {
		while (this.#list.length)
			this.#list[0].close();

		dnssd.count -= 1;
		if (0 === dnssd.count) {
			dnssd.close();
			dnssd = undefined;
		}
	}

	claim(options) {
		return new Claim(options, this.#list);
	}
	discover(options) {
		return new Discover(options, this.#list);
	}
	advertise(options) {
		return new Advertise(options, this.#list);
	}
}
