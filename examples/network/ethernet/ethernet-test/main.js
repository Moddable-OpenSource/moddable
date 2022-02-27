/*
 * Copyright (c) 2016-2021 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Net from "net";
import Ethernet from "ethernet";

try {
	Ethernet.start();
	let monitor = new Ethernet((msg, code) => {
		switch (msg) {
			case Ethernet.connected:
				trace("Physical link established...\n");
				break;

			case Ethernet.disconnected:
				trace("Ethernet disconnected.\n");
				break;

			case Ethernet.gotIP:
				let ip = Net.get("IP", "ethernet");
				trace(`Ethernet ready. IP address: ${ip}\n`);
				break;

			case Ethernet.lostIP:
				trace(`IP address lost. Network unavailable.\n`);
				break;
		}
	});
} catch (e) {
	debugger
}
