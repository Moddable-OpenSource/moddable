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

const HOSTNAMES = ["modSign", "anotherModSign"];

class Sign extends WebThing {
	constructor(host) {
		super(host);
		this.text = "";
	}
	get msg() {
		return this.text;
	}
	set msg(msg) {
		if (msg == this.text) return;
		this.text = msg;
		this.changed();
		application.distribute("updateMsg", msg);
	}
	static get description() {
		return {
			name: "sign",
			description: "Digital sign",
			properties: {
				msg: {
					type: "string",
					description: "Message",
					txt: "msg"
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
			trace("Error: no Wi-Fi credentials. Must include Wi-Fi credentials in $MODDABLE/modules/network/webthings/manifest.json\n");
			return;
		} else {
			wifiData.ssid = config.ssid;
			if (config.password) wifiData.password = config.password;
		}
		application.first.string = "Connecting to Wi-Fi...";
		new WiFi(wifiData, function(msg) {
			switch (msg) {
				case "connect":
					break; // still waiting for IP address
				case "gotIP":
					application.delegate("getHostname");
					this.close();
					return;
				case "disconnect":
					break;  // connection lost
			}
		});
	}
	getHostname(application) {
		application.first.string = "Getting hostname...";
		let index = 0;
		this.mdns = new MDNS({hostName: HOSTNAMES[index]}, (message, value) => {
			switch (message) {
				case 1:
					if ("" !== value) application.delegate("onHostname", value);
					break;
				case 2:
					index++;
					if (index < HOSTNAMES.length) {
						trace(`Failed to get hostname ${value}...\n`);
						return HOSTNAMES[index];
					} else {
						application.first.string = "Error: all hostnames taken";
						return -1;
					}
					break;
			}
		});
	}
	onHostname(application, value) {
		trace(`Got hostname ${value}.\n`);
		application.remove(application.first);
		application.add(new ASSETS.SignScreen(value));
		let mdns = this.mdns;
		let things = this.things = new WebThings(mdns);
		things.add(Sign);
	}
	onControllerUpdate(container, controllers) {
		let mdns = this.mdns;
		mdns.remove("_http._tcp");
		mdns.monitor("_http._tcp", (service, instance) => {
			let name = instance.name;
			let map = controllers[name];
			if (map == undefined) return;
			if (instance.txt.length == 0) return;
			let txt, txtKey, equal, key, value;
			for (let i in instance.txt) {
				txt = instance.txt[i];
				equal = txt.indexOf("=");
				txtKey = txt.substring(0, equal);
				key = map[txtKey];
				if (key != undefined) {
					value = txt.substring(equal+1);
					application.distribute("stateChanged", value);
				}
			}
		});
		return true;
	}
	stateChanged(container, state) {
		this.things.things[0].instance["msg"] = state;
	}
}

const SignApp = Application.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, Skin: ASSETS.MozillaBlueSkin,
	contents: [
		Text($, { left: 15, right: 15, Style: ASSETS.OpenSans28 }),
	],
	Behavior: AppBehavior
}));

export default function() {
	return new SignApp({}, {displayListLength: 2048});
}

