/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
import {WebThings, WebThing} from "webthings";
import config from "mc/config";
import WiFi from "wifi";
import MDNS from "mdns";
import ASSETS from "assets";
import Timer from "timer";

const HOSTNAMES = ["modThermostat", "anotherModThermostat"];

class Thermostat extends WebThing {
	constructor(host) {
		super(host);
		this.currentTemp = 65;
		this.targetTemp = 70;
	}
	set target(value) {
		value = Math.round(Number(value));
		if (value == this.targetTemp) return;
		if (value < 50) value = 50;
		else if (value > 100) value = 100;
		this.targetTemp = value;
		application.distribute("updateTarget", this.targetTemp, this.currentTemp);
	}
	get target() {
		return this.targetTemp;
	}
	get actual() {
		return this.currentTemp;
	}
	set actual(value) {

	}
	static get description() { 
		return {
			name: "thermostat",
			description: "pseudo thermostat",
			properties: {
				target: {
					type: "integer",
					description: "Desired temperature",
					minimum: 50,
					maximum: 100,
					unit: "fahrenheit",
					txt: "target",
				},
				actual: {
					type: "integer",
					description: "Current temperature",
					minimum: 50,
					maximum: 100,
					unit: "fahrenheit",
					txt: "actual",
					"readOnly": true,
				}
			}		
		}
	}
}

class AppBehavior extends Behavior {
	onCreate(application, data) {
		this.data = data;
	}
	onDisplaying(application) {
		let wifiData = {};
		if (config.ssid == "YOUR_WIFI_SSID_HERE") {
			application.first.string = "Error: no Wi-Fi credentials";
			trace("Error: no Wi-Fi credentials. Must include Wi-Fi credentials in manifest.json\n");
			return;
		} else {
			wifiData.ssid = config.ssid;
			if (config.password) wifiData.password = config.password;
		}
		application.first.string = "Connecting to Wi-Fi...";
		new WiFi(wifiData, msg => {
			switch (msg) {
				case "connect":
					break; // still waiting for IP address
				case "gotIP":
					this.getHostname(application);
					return;
				case "disconnect":
					break;  // connection lost
			}
		})
	}
	getHostname(application) {
		application.first.string = "Getting hostname...";
		let index = 0;
		this.mdns = new MDNS({hostName: HOSTNAMES[index]}, (message, value) => {
			switch (message) {
				case 1:
					if ("" !== value) this.onHostname(application, value);
					break;
				case 2:
					index++;
					if (index < HOSTNAMES.length) {
						trace(`Failed to get hostname ${value}...\n`);
						application.first.string = "Error: all hostnames taken";
						return HOSTNAMES[index];
					} else {
						trace(`Failed to get a hostname. Stopping.\n`);
						return -1;
					}
					break;
			}
		});
	}
	onHostname(application, value) {
		trace(`Got hostname ${value}.\n`);
		this.data.hostname = value;
		Timer.set(() => {
			application.remove(application.first);
			application.add(new ASSETS.ThermostatScreen(this.data));
		}, 0);
		let mdns = this.mdns;
		let things = this.things = new WebThings(mdns);
		things.add(Thermostat);
		application.interval = 6000;
		application.start();
	}
	onTimeChanged(container) {
		let thermostat = this.things.things[0].instance;
		let temp = thermostat.currentTemp;
		let target = thermostat.target;
		if (temp < target) temp += 1;
		else temp -= 1;
		thermostat.currentTemp = temp;
		this.updateActual(application, target, temp);
	}
	incrementTarget(container, delta) {
		this.things.things[0].instance["target"] += delta;
	}
	updateActual(container, target, temp) {
		this.data["ACTUAL"].string = String(temp)+"°";
		this.data["UNDERLINE"].delegate("update", temp < target);
	}
	updateTarget(container, target, temp) {
		this.data["TARGET"].string = String(target)+"°";
		this.data["UNDERLINE"].delegate("update", temp < target);
	}
}

const ThermostatApp = Application.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, Skin: ASSETS.MozillaBlueSkin,
	contents: [
		Text($, { left: 15, right: 15, Style: ASSETS.OpenSans20 }),
	],
	Behavior: AppBehavior
}));

export default function() {
	return new ThermostatApp({});
}
