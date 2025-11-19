/*
 * Copyright (c) 2025 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Timer from "timer";

const dnssd = new (device.network.dnssd.io)(device.network.dnssd);

dnssd.claim({
	host: "a-server",
	onReady() {
		trace(`a-server claimed\n`);
	},
	onError() {
		trace("couldn't claim a-server\n");
	}
});

const ad = dnssd.advertise({
	serviceType: "_http._tcp",
	name: "419 Web Server",
	host: "a-server",
	port: 8080,
	txt: new Map([
		["home", "/index.html"]
	]),
	onError(/* error */) {
		trace("advertise failed\n");
	}
});
ad.counter = 0;

Timer.repeat(() => {
	ad.updateTXT(new Map([
		["home", "/index.html"],
		["counter", (++ad.counter).toString()],
	]));
}, 1000)

dnssd.discover({
	serviceType: "_airplay._tcp",
	onFound(service) {
		trace(`Found: "${service.name}" on ${service.host} @ ${service.address}:${service.port}\n`);
		this.history ??= new Map;
		const txt = service.txt ?? new Map;
		this.history.set(service.host, txt); 
		for (const [key, value] of txt)
			trace(`  ${key}:${value}\n`);
	},
	onUpdate(service) {
		trace(`Update: ${service.name}\n`);

		const previous = this.history.get(service.host);
		const txt = service.txt ?? new Map;
		this.history.set(service.host, txt);

		for (const [key, value] of previous) {
			if (!txt.has(key))
				trace(`  removed ${key}:${value}\n`);
		}
		for (const [key, value] of txt) {
			if (!previous.has(key))
				trace(`  added ${key}:${value}\n`);
		}
		for (const [key] of txt) {
			if (previous.has(key) && (previous.get(key) !== txt.get(key)))
				trace(`  changed ${key} from ${previous.get(key)} to ${txt.get(key)}\n`);
		}
	},
	onLost(service) {
		trace(`Lost: ${service.name}\n`);
		this.history.delete(service.host);
	},
	onError(/* error */) {
		trace(`failed\n`);
	}
});
