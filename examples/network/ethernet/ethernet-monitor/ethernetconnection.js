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

import Ethernet from "ethernet";

class EthernetMonitor {
	constructor() {
		globalThis.ethernet = { connected: false };

		Ethernet.start();
		this.monitor = new Ethernet((msg, code) => {
			switch (msg) {
				case Ethernet.gotIP:
					ethernet.connected = true;
					break;

				case Ethernet.disconnected:
					ethernet.connected = false;
					break;
			}
		});	
	}
}

export default EthernetMonitor;