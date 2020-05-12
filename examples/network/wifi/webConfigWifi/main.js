/*
 * Copyright (c) 2019  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Timer from "timer";
import WiFi from "wifi";
import {Server} from "http"
import Net from "net"

import MDNS from "mdns";
import Preference from "preference";

const HTML_TITLE = "Set SSID";
const AP_NAME = "myAccessPoint";
const AP_PASSWORD = "12345678";
let hostName = "mywifi";

const PREF_WIFI = "wifi";
let stored_ssid = Preference.get(PREF_WIFI, "ssid");
let stored_pass = Preference.get(PREF_WIFI, "password");

let accessPointList = [];

class WebConfigWifi {
	constructor(dict) {
		this.dict = dict;
		this.connecting = 0;
		this.usingAP = 0;

		if (undefined === this.dict.ssid || '' === this.dict.ssid) {
			this.configAP(AP_NAME, AP_PASSWORD);
		}
		else {
			Timer.set(id => { this.checkConnected(); }, 10000);
			this.doWiFiScan();
		}
	}

	doWiFiScan() {
		trace(`doWifiScan - looking for ${this.dict.ssid}\n`);
		WiFi.scan({}, item => {
			if (item) {
				let theap = accessPointList.find(x => x.ssid === item.ssid);
				if (undefined === theap)
					accessPointList.push(item);
				else
					theap.rssi = item.rssi;

				if (item.ssid == this.dict.ssid)
					this.checkSSIDs();
			}
		});
	}

	checkSSIDs() {
		trace(`checkSSIDs - looking for ${this.dict.ssid}\n`);

		let theap = accessPointList.find(x => x.ssid === this.dict.ssid);
		if (undefined !== theap)
			this.connect(this.dict.ssid, this.dict.password);
		else {
			if (WiFi.status === 0) {			// idle
				if (this.wifiScans++ > MAX_WIFI_SCANS) {
					this.wifiScans = 0;
					this.checkConnected();
				}
				else
					this.doWiFiScan();
			}
		}
	}

	connect(ssid, password) {
		trace(`connect - ${ssid} ${password}\n`);
		this.connecting = 1;

		let tries = 0;
		this.myWiFi = new WiFi({ssid, password}, msg => {
			switch (msg) {
				case "connect":
					break;
				case "gotIP":
					if (!this.connectionWasEstablised) {
						this.connected();
					}
					break;
				case "disconnect":
					this.connecting = 0;
					if (undefined === WiFi.status) {
						this.checkConnected();
					}
					//@@
					break;
			}
		});
	}

	connected() {
		trace(`connected\n`);
		this.connecting = 0;
		this.connectionWasEstablished = 1;

		this.configServer();
	}

	checkConnected() {
		trace(`checkConnected\n`);
		if (this.connectionWasEstablished || this.connecting || (undefined !== this.usingAP))
			return;
		if (undefined !== this.myWiFi) {
			this.myWiFi.close();
			this.myWiFi = undefined;
		}
		WiFi.close();
		this.configAP(AP_NAME, AP_PASSWORD);
	}

	advertiseServer() {
		trace(`advertiseServer ${this.dict.name}\n`);
		this.mdns = new MDNS({hostName: this.dict.name}, function(message, value) {
			if (1 === message) {
				if ('' != value && undefined !== this.owner) {
					this.owner.dict.name = value;
				}
			}
		});
		this.mdns.owner = this;
	}

	configServer() {
		trace(`configServer\n`);
		this.head = `<html><head><title>${HTML_TITLE}</title></head>`;

		this.apServer  = new Server();
		this.apServer.owner = this;
		this.apServer.callback = function(message, value, v2) {
			switch (message) {
				case Server.status:
					this.userReq = [];
					this.path = value;
					break;

				case Server.headersComplete:
					return String;

				case Server.requestComplete:
					let postData = value.split("&");
					for (let i=0; i<postData.length; i++) {
						let x = postData[i].split("=");
						this.userReq[x[0]] = x[1].replace("+"," ");
					}
					break;

				case Server.prepareResponse:
					let msg;
					if (this.path == "/set-ssid") {
						if (this.userReq.ssid) {
							msg = this.server.owner.responsePageSSID(this.userReq);
						}
					}
					else {
						if (this.server.owner.usingAP)
							msg = this.server.owner.requestPageSSID();
						else
							msg = this.server.owner.requestPage();
					}

					return {headers: ["Content-type", "text/html"], body: msg};
					break;

				case Server.responseComplete:
					if (this.path == "/set-ssid" && undefined !== this.userReq) {
						if (undefined !== this.userReq.ssid) {
							Preference.set(PREF_WIFI, "ssid", this.userReq.ssid);
							Preference.set(PREF_WIFI, "password", this.userReq.password);
							doRestart();
						}
					}
					else if (this.path == "/reset") {
						trace("resetting at user request\n");
						Preference.set(PREF_WIFI, "ssid", "");
						Preference.set(PREF_WIFI, "password", "");
						doRestart();
					}
					break;
			}
		}

		this.advertiseServer();
	}

	responsePageSSID(req) {
		let msg = `<h3>Attempting to connect to <b>${req.ssid}</b></h3>
In awhile, reconnect to ${req.ssid} and visit <a href="http://${this.dict.name}.local/">http://${this.dict.name}.local/</a>`;
		return msg;
	}

	requestPage() {
		let msg = `<h1>myServer at ${Net.get("IP")}</h1>
<form action="/reset" method="post">
<div class="button"><button>Reset</button></div></form><p>
Reset will clear the saved ssid and password and reboot.<p>
Then reconnect to the access point "${AP_NAME}" and visit <a href="http://${this.dict.name}.local/">http://${this.dict.name}.local/</a> to set the ssid and password again.`;

		return msg;
	}

	requestPageSSID() {
		let ssid = (stored_ssid === undefined) ? "" : stored_ssid;
		return `<html><head><title><${HTML_TITLE}></title></head><body>
<h2>Please configure an access point.</h2>
<form action="/set-ssid" method="post">
<div><label name="ssid">SSID: </label><input type="text" id="ssid" name="ssid" value="${ssid}"><p>
<label name="password">Password: </label><input type="password" id="password" name="password" minlength="8"><p></div>
<div class="button"><button>Set SSID</button></div></form>`;
	}

	configAP(ssid, password) {
		trace(`configAP\n`);
		this.usingAP = 1;

		if (undefined !== this.myWiFi) {
			this.myWiFi.close();
			this.myWiFi = undefined;
		}
		this.AP = WiFi.accessPoint({ ssid, password });
		this.configServer();
	}
}


function restart() @ "do_restart";

function doRestart() {
	trace(`restarting in 1 second.\n`);
    Timer.delay(1000);
    restart();
}

let setupWifi = new WebConfigWifi({ ssid:stored_ssid, password:stored_pass, name:hostName});

