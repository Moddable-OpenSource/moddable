/*
 * Copyright (c) 2016-2025 Moddable Tech, Inc.
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
			if (config.ethernet.static) {
				Ethernet.useStaticIP(config.ethernet.static.address, config.ethernet.static.mask, config.ethernet.static.gateway);
				trace(`EthernetMonitor-using Static IP: ${config.ethernet.static.address}\n`);
			}
			else {
				Ethernet.useDHCP();		// this is the default
				trace(`EthernetMonitor-using DHCP\n`);
			}
		} catch (error) {
			trace(`EthernetMonitor-Ethernet initializaiton failed: ${error}\n`);
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
