/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

import {} from "piu/MC";
import Net from "net";
import SNTP from "sntp";
import WiFi from "wifi";
import Time from "time";
import {
	WiFiStatusSpinner
} from "assets";
import {
	NetworkListScreen,
	LoginScreen,
	ConnectionErrorScreen
} from "wifi-screens";

function getVariantFromSignalLevel(value) {
	let low = -120;
	let high = -40;
	if (value < low) value = low;
	if (value > high) value = high;
	return Math.round(4 * ((value - low) / (high - low)));
}

class ApplicationBehavior extends Behavior {
	onCreate(application) {
		global.application = application;
		WiFi.mode = 1;
		this.doNext(application, "NETWORK_LIST_SCAN");
		application.interval = 2000;
		application.start();
	}
	onTimeChanged(application) {
		let now = Date.now();
		if ((this.timeout) && (now > this.timeout)) {
			trace("Attempt to connect to Wi-Fi timed out\n");
			if (global.monitor) {
				global.monitor.close();
				global.monitor = undefined;
			}
			WiFi.connect();
			application.delegate("doNext", "CONNECTION_ERROR", this.nextScreenData);
			delete this.timeout;
			delete this.nextScreenData;
		}
	}
	doNext(application, nextScreenName, nextScreenData = {}) {
		application.defer("onSwitchScreen", nextScreenName, nextScreenData);
	}
	onSwitchScreen(application, nextScreenName, nextScreenData = {}) {
		if (application.length) application.remove(application.first);
		application.purge();
		switch (nextScreenName) {
			case "NETWORK_LIST_SCAN":
				// expected nextScreenData: {}
				if (undefined === this.networks) {
					application.add(new WiFiStatusSpinner({ status: "Finding networks..." }));
					this.scan(application, true);
				} else {
					application.add(new NetworkListScreen({networks: this.networks}));
				}
				break;
			case "NETWORK_LIST":
				// expected nextScreenData: { networks: x } where x is a linked list created above^
				// or { networks: x, ssid: "y" }
				application.add(new NetworkListScreen(nextScreenData));
				break;
			case "LOGIN":
				application.add(new LoginScreen(nextScreenData));
				break;
			case "CONNECTING":
				WiFi.connect();
				application.add(new WiFiStatusSpinner({ status: "Joining network..." }));
				this.timeout = Date.now() + 10000;
				this.nextScreenData = nextScreenData;
				global.monitor = new WiFi(nextScreenData, message => {
					if (message == "gotIP"){
					  	Net.resolve("pool.ntp.org", (name, host) => {
							if (!host) {
								trace("Unable to resolve sntp host\n");
								application.delegate("doNext", "CONNECTION_ERROR", nextScreenData);
								return;
							}
							trace(`resolved ${name} to ${host}\n`);
							global.application.behavior.timeout = Date.now() + 10000;
							let sntp = new SNTP({host}, (message, value) => {
								if (1 == message) {
									Time.set(value);
									delete global.application.behavior.timeout;
									if (application.behavior.remoteControlEnabled) application.delegate("wsConnect", nextScreenData);
									application.delegate("doNext", "NETWORK_LIST", { networks: application.behavior.networks, ssid: nextScreenData.ssid });
								} else if (-1 == message) {
									trace("Unable to retrieve time\n");
									application.delegate("doNext", "CONNECTION_ERROR", nextScreenData);
								} else {
									return;
								}
							});
						});
						return;
					} else if (message == "disconnect") {
						return;
					}
				});
				break;
			case "CONNECTION_ERROR":
				application.add(new ConnectionErrorScreen(nextScreenData));
				break;
			case "SET_TIMEZONE":
				// expected nextScreenData: { current: x } where x is the name of the current timezone, as a string
				application.add(new SetTimezoneScreen(nextScreenData));
				break;
			case "REMOTE_CONTROL_SETTINGS":
				// expected nextScreenData: {}
				nextScreenData.enabled = this.remoteControlEnabled;
				nextScreenData.wsConnected = (this.ws != undefined)? 1: 0;
				application.add(new RemoteControlSettingsScreen(nextScreenData));
				break;
		}
	}
	scan(application, isFirstScan) {
		WiFi.scan({}, item => {
			let networks = application.behavior.networks;
			if (item) {
				let strength = getVariantFromSignalLevel(item.rssi);
				for (let walker = networks; walker; walker = walker.next) {
					if (walker.ssid === item.ssid) {
						if (strength > Math.abs(walker.variant))
							walker.variant = strength * Math.sign(walker.variant);
						return;
					}
				}
				let ap = { ssid: item.ssid, variant: (item.authentication === "none") ? -strength : strength, next: networks };
				application.behavior.networks = ap;
			} else {
				if (isFirstScan) application.delegate("doNext", "NETWORK_LIST", { networks });
				else application.first.distribute("onUpdateNetworkList", networks);
			}
		});
	}
}
Object.freeze(ApplicationBehavior.prototype);

export default function() {
	return new Application(null, {
		displayListLength: 2560, commandListLength: 2048, touchCount: 1, Behavior: ApplicationBehavior
	});
}
