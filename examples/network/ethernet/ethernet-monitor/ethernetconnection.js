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
import config from "mc/config";
import Ethernet from "ethernet";
import Net from "net";

class EthernetMonitor {
	constructor() {
		if (config.ssid !== undefined) {
			trace("EthernetMonitor-Skipping Ethernet setup because Wi-Fi SSID is configured.\n");
			return done();
		}
		globalThis.ethernet = { connected: false };

		try {
			Ethernet.start();
			//Ethernet.setStaticIP("192.168.1.190", "255.255.255.0", "192.168.1.1");
			//trace(`EthernetMonitor-Static IP set: 192.168.1.190\n`);
			Ethernet.doDHCP();
			trace(`EthernetMonitor-DHCP started\n`);
		} catch (error) {
			trace(`EthernetMonitor-Ethernet hardware not found. ${error}\n`);
			return done();
		}
		
		trace(`EthernetMonitor-Waiting for Ethernet link.\n`);
		this.monitor = new Ethernet((msg, code) => {
			switch (msg) {
				case Ethernet.gotIP:
					trace(`EthernetMonitor-IP address ${Net.get("IP", "ethernet")} MAC address: ${Net.get("MAC", "ethernet")}\n`);
					ethernet.connected = true;
					break;

			case "connect":
				trace(`EthernetMonitor-Ethernet connected\n`);
				break;

			case "disconnect":
				ethernet.connected = false;
				trace(`EthernetMonitor-Ethernet disconnected\n`);
				break;
			}
		});	
	}
}

export default EthernetMonitor;