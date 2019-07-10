/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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

/*
	ToDo:
		Access control
		secure preferences
		connect with Android devices (use BT beacon to advertise address?)

		better html
		
	Future:
		extended strand effects

	Opportunities/projects:
		alarm
			mp3 player
			internet radio
			physical bell
			snooze button
		Time/temp/barometer
		Second display expansion (time + temp)
		automatically use an open access point (ie. no password)
*/

import config from "mc/config";

import Timer from "timer";
import Time from "time";
import WiFi from "wifi";
import {Request, Server} from "http";
import Net from "net";
import SNTP from "sntp";
import Resource from "Resource";
import MDNS from "mdns";
import Preference from "preference";
import Monitor from "pins/digital/monitor";
import Digital from "pins/digital";

import SevenSegDisplay from "sevenseg";
import ClockStyle from "styles";
import html_content from "web";
import OTARequest from "update";

import ClockPrefs from "prefs";

const ntpHosts = ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org", "3.pool.ntp.org"];

const PROD_NAME = "ModClock";
const UPDATE_URL = "http://robotranch.org/superclock/clock.update.current";
const LOCAL_SCRIPTS = "http://192.168.4.1/";
const REMOTE_SCRIPTS = "http://robotranch.org/superclock/";

const AP_NAME = "clock";
const AP_PASSWORD = "12345678";

const MAX_WIFI_SCANS = 3;		// before opening the access point

const BUTTON_PIN = 0;

const BUTTON_DEBOUNCE_MS = 30;
const BUTTON_CHANGE_TIME_MS = 1000;		// hold > 2s to set time
const BUTTON_ACCEPT_TIME_MS = 2500;		// hold > 4s to accept time
const BUTTON_ERASE_WARNING_TIME_MS = 8000;		// blink like mad at 8s to warn of imending erasure
const BUTTON_ERASE_PREFS_MS = 10000;	// hold 10 s to erase prefs

let hostName = "clock";

let prefs = new ClockPrefs();

trace(`Clock starting... Name: ${prefs.name}\n`);

Time.set(0);

let accessPointList = [];
const RESET_TIME_INTERVAL = (1000 * 60 * 60 * 12);		// 12 hours
let favico = new Resource("favicon.ico");
//let colorPicker = new Resource("html5kellycolorpicker.ico");
//let colorPicker = undefined;

/*
const Timing_WS2812B = {
    mark:  { level0: 1, duration0: 900,  level1: 0, duration1: 350, },
    space: { level0: 1, duration0: 350,   level1: 0, duration1: 1000, },
    reset: { level0: 0, duration0: 30000, level1: 0, duration1: 30000, } };
*/

const Timing_WS2811 = {
    mark:  { level0: 1, duration0: 950,  level1: 0, duration1: 350, },
    space: { level0: 1, duration0: 350,   level1: 0, duration1: 950, },
    reset: { level0: 0, duration0: 50000, level1: 0, duration1: 50000, } };

class Clock {
	constructor(prefs) {
		this.prefs = prefs;
		prefs.owner = this;
		this.display = new SevenSegDisplay( {
			length:58, pin:prefs.pin,
			order:config.seven_segments[prefs.layout].order,
			tail_order:prefs.tail_order,
			timing:Timing_WS2811,
			layout:prefs.layout,
			extra:prefs.extra,
			twelve:prefs.twelve, 
			brightness:prefs.brightness,
			tail_brightness:prefs.tail_brightness } );

		this.styles = [
			new ClockStyle.OneColor(this.display, {}),
			new ClockStyle.TwoColor(this.display, {}),
			new ClockStyle.Rainbow(this.display, {}),
			new ClockStyle.ColorWheel(this.display, {}),
			new ClockStyle.NiceColors(this.display, {}),
			new ClockStyle.Special(this.display, {})
		];
		this.styles.forEach(prefs.loadPref);

		this.currentStyle = this.styles.find(x => x.tag === prefs.style);
		if (undefined === this.currentStyle)
			this.currentStyle = this.styles[0];
		this.display.value("helo").blink();

		this.wifiScans = 0;
		this.connecting = 0;
		this.usingAP = undefined;

//		if (undefined === this.prefs.ssid) {
//			this.configAP(AP_NAME, AP_PASSWORD);
//		}
//		else
		 {
			this.display.value("scan").blink();
			Timer.set(id => { this.checkConnected(); }, 10000);		// wait ten seconds before falling back to AP
			this.doWiFiScan(this.prefs.ssid);
		}

		this.monitor = new Monitor({ pin: BUTTON_PIN, mode: Digital.InputPullUp, edge: Monitor.Falling | Monitor.Rising });
		this.monitor.idx = 0;
		this.monitor.lastPressed = Time.ticks;
		this.monitor.timePressed = 0;
		this.monitor.stillDown = 0;
		this.monitor.changeDigits = 0;
		this.monitor.onChanged = this.buttonPressed;
		this.monitor.clock = this;
	}

	getbuildstring() @ "xs_getbuildstring";

	get currentStyle() { return this._style; }
	set currentStyle(v) { this._style = v; this.display.style = v; return v; }

	doWiFiScan(ssid="") {
		WiFi.scan({prefs:this.prefs}, item => {
			if (item) {
				let theap = accessPointList.find(x => x.ssid === item.ssid);
				if (undefined === theap)
					accessPointList.push(item);
				else
					theap.rssi = item.rssi;
	
				if (item.ssid == this.prefs.ssid)
					this.checkSSIDS();
			}
		});
	}

	checkSSIDS() {
		let theap = accessPointList.find(x => x.ssid === this.prefs.ssid);
		if (undefined !== theap)
			this.connect(this.prefs.ssid, this.prefs.pass);
		else {
			if (WiFi.status === 0) {			// idle
				if (this.wifiScans++ > MAX_WIFI_SCANS) {
					this.wifiScans = 0;
					this.checkConnected();
				}
				else
					this.doWiFiScan(this.prefs.ssid);
			}
		}
	}

	connect(ssid, password) {
		this.display.value("conn").blink();
		this.connecting++;

		trace(`Starting WiFi to connect to <${ssid}>...\n`);
		
		let tries = 0;

		this.myWiFi = new WiFi({ssid, password}, msg => {
			switch (msg) {
				case "connect":
					trace(`WiFi connecting to ${ssid}\n`);
					break;
				case "gotIP":
					if (!this.connectionWasEstablished) {
						trace(`WiFi gotIP: ${Net.get("IP")}. Going to connect to the local network.\n`);
						this.connected();
					}
					break;
				case "disconnect":
					if (undefined === WiFi.status) {
						this.display.value("fail").blink();
						if (this.connecting < 2) {
							this.myWiFi.close();
							this.myWiFi = undefined;
							this.connect(ssid, password);
						}
						else {
							this.connecting = 0;
							this.checkConnected();
						}
					}
					else {		// do we get here?
							this.connecting = 0;
							this.display.value("fa-x").blink();
							if (tries >= 1) {
								this.myWiFi.close();
								this.myWiFi = undefined;
								if (this.connectionWasEstablished) {
									this.connectionWasEstablished = 0;
									trace("WiFi disconnected - try to reconnect\n");
									this.connect(ssid, password);
								}
								else {
									trace("WiFi never established - ask for SSID\n");
									this.configAP(AP_NAME, AP_PASSWORD);
								}
							}
					}
					break;
			}
		});
	}

	connected() {
		this.connecting = 0;
		this.connectionWasEstablished = 1;
		this.display.value("yay").blink();

		this.fetchTime();
		this.configServer();
	}

	checkConnected() {
		if (this.connectionWasEstablished || this.connecting || (undefined !== this.usingAP))
			return;
		if (undefined !== this.myWiFi) {
			this.myWiFi.close();
			this.myWiFi = undefined;
		}
		this.configAP(AP_NAME, AP_PASSWORD);
	}

	advertiseServer() {
		let name = this.prefs.name;
		if (this.usingAP)
			name = AP_NAME;

		this.mdns = new MDNS({hostName: name, prefs: this.prefs}, function(message, value) {
			if (1 === message) {
				if ('' != value) {
					if (value != clock.prefs.name) {
						clock.prefs.name = value;
					}
				}
			}
		});
		this.mdns.clock = this;
	}

	checkName(newName, oldName) {
		if ((undefined !== newName) && (newName.length > 3) && (newName != oldName)) {
			return newName.replace(/\s+/g,"_").toLowerCase();
		}
		return 0;
	}

	checkPass(newPass, oldPass) {
		if ((undefined !== newPass) && (newPass.length > 7) &&  (oldPass != newPass))
			return 1;
		return 0;
	}

	configServer() {
		this.head = `<html><head><title>${PROD_NAME}</title></head>`;
		this.suffixLine = "build: " + this.getbuildstring();

		this.uiServer = new Server();
		this.uiServer.clock = this;
		this.uiServer.callback = function(message, value, v2) {
			let clock = this.server.clock;
			let name = clock.prefs.name;
			switch (message) {
				case Server.status:
					this.userReq = [];
					this.path = value;
					this.redirect = 0;
					break;

				case Server.headersComplete:
					return String;

				case Server.requestComplete:
					let postData = value.split("&");

					for (let i=0; i<postData.length; i++) {
						let x = postData[i].split("=");
						this.userReq[x[0]] = x[1].replace("+"," ");
					}
					this.redirect = 1;
					if (this.path == "/style") {
						let newStyle = clock.styles.find(x => x.tag === this.userReq.style);
						newStyle.options_response(this.userReq);
						this.redirect = 1;

						clock.prefs.style = this.userReq.style;
						clock.styles.forEach(clock.prefs.savePref);

						clock.prefs.brightness = parseInt(this.userReq.brightness);
						clock.prefs.tail_brightness = parseInt(this.userReq.tail_brightness);
						clock.prefs.tail_order = clock.display.supportedFormats[this.userReq.tail_order]
						clock.prefs.extra = parseInt(this.userReq.extra);
					}
					else if (this.path == "/rescanSSID" || this.path == "/html5kellycolorpicker.min.js" || this.path.slice(0,19) == "/current_version.js")
						this.redirect = 0;
					else if (this.path == "/set-ssid") {
						if (!this.userReq.ssid)
							this.userReq.ssid = this.userReq.ssid_select;
						this.redirect = 0;
					}
					break;

				case Server.prepareResponse:
					let msg;

					if (this.userReq) {
						name = clock.checkName(this.userReq.clock_name, clock.prefs.name);
						if (0 === name)
							name = clock.prefs.name;
						else {
							msg = `${html_content.redirectHead(name, 90)} ${html_content.bodyPrefix() } Changing clock name to <b>${name}</b>.<br>Please allow up to 90 seconds for restart.<p><a href="http://${name}.local">http://${name}.local</a><p>${html_content.bodySuffix(this.server.clock.suffixLine)}`;
							this.redirect = 0;
						}
					}

					if (this.redirect) {
						msg = html_content.redirectHead(clock.prefs.name, 0) + html_content.bodyPrefix() + "One moment please." + html_content.bodySuffix(this.server.clock.suffixLine);
					}

					if (this.path == "/set-ssid") {
						if (this.userReq.ssid) {
							msg = this.server.clock.head + html_content.bodyPrefix() + html_content.changeSSIDResp(this.userReq.ssid, name) + html_content.bodySuffix(this.server.clock.suffixLine);
						}
						trace("new ssid requested");
					}
					else if (this.path == "/html5kellycolorpicker.min.js") {
//						return {headers: ["Cache-Control", "public, max-age=31536000"], body: colorPicker.slice(0)};
						return {headers: ["Cache-Control", "public, max-age=31536000"], body: "class KellyColorPicker { }"};
					}
					else if (this.path.slice(0,19) == "/current_version.js") {
						return {headers: ["Cache-Control", "public, max-age=31536000"], body: "function current_version() { return 'current';} function check_update(version) { return false; }" };
					}
					else if (this.path == "/favicon.ico") {
						return {headers: ["Content-type", "image/vnd.microsoft.icon", "Cache-Control", "public, max-age=31536000"], body: favico.slice(0)};
					}
					else if (this.path == "/reset") {
						msg = this.server.clock.head + html_content.bodyPrefix() + html_content.resetPrefsResp();
					}
					else if (!this.redirect && undefined === msg) {
						msg = this.server.clock.head + html_content.bodyPrefix();
						if (clock.connectionWasEstablished)
 							msg += html_content.clockScripts(clock.currentStyle, REMOTE_SCRIPTS);
						else
 							msg += html_content.clockScripts(clock.currentStyle, LOCAL_SCRIPTS);
						
						msg += `<h1>${PROD_NAME}</h1>`;

						if (undefined !== clock.ota)
							msg += `<b>Updating</b> - Received ${clock.ota.received} of ${clock.ota.length === undefined ? "unknown" : clock.ota.length}<p>`;

						if (!clock.connectionWasEstablished)
							msg += `${html_content.noAccessPointSet()} ${html_content.accessPointSection(accessPointList, clock.prefs.ssid, clock.prefs.name)}<hr>`;

						msg += `${html_content.clockStyleSection(clock)}<hr>${html_content.clockOptionsSection(clock.prefs)}<hr>`;

						if (clock.connectionWasEstablished)
							msg += `${html_content.accessPointSection(accessPointList, clock.prefs.ssid, clock.prefs.name)}<hr>`;

						msg += `${html_content.clockUpdateCheck(clock)} <hr>${html_content.clockResetPrefsSection(clock)} ${html_content.bodySuffix(this.server.clock.suffixLine)}`;

					}
					if (undefined !== msg && (msg.length > 1024)) {
						this.msg = msg;
						msg = true;
					}
					return {headers: ["Content-type", "text/html"], body: msg};
					break;

				case Server.responseFragment:
					let ret = this.msg;
					if (undefined === this.msg) {
					}
					else if (this.msg.length > 1024) {
						ret = this.msg.slice(0, 1024);
						this.msg = this.msg.slice(1024);
					}
					else {
						ret = this.msg;
						this.msg = undefined;
					}
					return ret;
	
				case Server.responseComplete:
					let resetTime = 0;
					let resetWiFi = 0;
					if (this.path == "/set-ssid") {
						if ((undefined !== this.userReq.ssid) && (clock.prefs.ssid != this.userReq.ssid)) {
							clock.prefs.ssid = this.userReq.ssid;
							resetWiFi = 1;
						}

						if (clock.checkPass(this.userReq.password, clock.prefs.pass)) {
							clock.prefs.pass = this.userReq.password;
							resetWiFi = 1;
						}
						if (0 != (name = clock.checkName(this.userReq.clock_name, clock.prefs.name))) {
							clock.prefs.storedName = name;
							resetWiFi = 1;
						}
					}
					else if (this.path == "/clockOptions") {
						clock.prefs.tz = this.userReq.timezone;

						if (clock.checkName(this.userReq.clock_name, clock.prefs.name)) {
							clock.prefs.storedName = this.userReq.clock_name;
							resetWiFi = 1;
						}

				// checkboxes return no value if they aren't checked. So undefined === 0

						clock.prefs.pin = parseInt(this.userReq.pin);
						clock.prefs.layout = parseInt(this.userReq.layout);
						clock.prefs.dst = checkboxValue(this.userReq, "dst");
						clock.prefs.twelve = checkboxValue(this.userReq, "twelve");
					}
					else if (this.path == "/reset") {
						clock.prefs.reset();
						trace("restarting after resetting preferences\n");
						doRestart(this.clock);
					}
					else if (this.path == "/checkForUpdate") {
						clock.ota = new OTARequest(UPDATE_URL);
						clock.ota.onFinished = restart;
						clock.currentStyle = clock.styles[0];			// change to default style to indicate updating
					}
					else if (this.path == "/rescanSSID") {
						clock.doWiFiScan();
					}
	
					if (resetWiFi) {
						Timer.set(id => { doRestart(clock); }, 1000);		// wait a second for the response to be sent
//						doRestart(this.clock);
					}
					else {
						if (resetTime)
							clock.fetchTime();
					}
					break;
			}
		}

		trace(`clock's http server ready at ${Net.get("IP")}\n`);
		this.advertiseServer();
	}

	configAP(ssid, password) {
trace(`Configure access point ${ssid}\n`);
		this.usingAP = 1;
		this.display.value("set ").blink();

		if (undefined !== this.myWiFi) {
			this.myWiFi.close();
			this.myWiFi = undefined;
		}
		this.AP = WiFi.accessPoint({ ssid, password });

        this.fetchTime();

		this.configServer();
	}

	fetchTime() {
		if (!this.connectionWasEstablished)
			return;

trace(`fetchTime\n`);
		if (undefined !== this.upToDateTimer) {
			Timer.schedule(this.upToDateTimer, 100, RESET_TIME_INTERVAL);
			return;
		}

		this.upToDateTimer = Timer.set(id => {
			let hosts = Object.assign([], ntpHosts);
			let sntp = new SNTP({host: hosts.shift()}, function(message, value) {
				switch (message) {
					case 1:			// success!
						let tz = this.clock.prefs.tz + this.clock.prefs.dst - 11;
						Time.timezone = tz * 3600;
						Time.set(value + Time.timezone);
						this.clock.display.showTime(this.clock.currentStyle);
						break;
					case -1:
						if (hosts.length)
							return hosts.shift();
						break;
				}
			});
			sntp.clock = this;
		}, 100, RESET_TIME_INTERVAL);
	}

	buttonPressed() {
		let now = Time.ticks;
		if (! this.clock.display.timeShowing)
			this.clock.display.showTime(this.clock.currentStyle);
			

		let d = new Date();
		let timeVal = d.getHours() * 100 + d.getMinutes();
		let h = (timeVal / 100) | 0;
		let m10 = ((timeVal % 100) / 10) | 0;
		let m1 = (timeVal % 10) | 0;
		let elapsed = 0;

		if (this.read()) {				// if high then button is released
			if (this.timePressed)
	 			elapsed = now - this.timePressed;

			trace(`buttonReleased ${elapsed} ms\n`);
			this.stillDown = 0;
			if (undefined !== this.stillDownTimer)
				Timer.clear(this.stillDownTimer);
			if (elapsed > BUTTON_ERASE_PREFS_MS) {
				trace(`held down for ${now - this.timePressed} ms. Erase prefs and restart\n`);
				this.clock.prefs.reset();
				doRestart(this.clock);
			}
			else if (elapsed > BUTTON_ACCEPT_TIME_MS) {
				trace(`accepted.\n`);
				this.blinking = 0;
				this.changeDigits = 0;
				this.clock.display.showTime(this.clock.currentStyle);
			}
			else if (elapsed > BUTTON_CHANGE_TIME_MS) {
				if (1 == this.changeDigits) {
					trace(`change to minute 10s\n`);
					this.changeDigits = 2;
					this.clock.display.blink(300,0x2);
				}
				else if (2 == this.changeDigits) {
					trace(`change to minute 1s\n`);
					this.changeDigits = 3;
					this.clock.display.blink(300,0x1);
				}
				else if (3 == this.changeDigits) {
					trace(`change to hours\n`);
					this.changeDigits = 1;
					this.clock.display.blink(300,0xc);
				}
			}
			else if (elapsed > BUTTON_DEBOUNCE_MS) {
				if (0 == this.changeDigits) {
					this.idx = (this.idx + 1) % this.clock.styles.length;
					this.clock.prefs.style = this.idx;
				}
				else {							// we're setting the time
				let nowTime = Date.now() / 1000;
				if (1 == this.changeDigits) {
					trace(`change hours digit\n`);
					h += 1;
					if (h > 23) {
						h = 0;
						nowTime -= (60 * 60) * 23;
					}
					else
						nowTime += 60 * 60;
				}
				else if (2 == this.changeDigits) {
					trace(`change minute 10s digit\n`);
					m10 += 1;
					if (m10 > 10) {
						m10 = 1;
						nowTime -= 60 * 60;
					}
					else
						nowTime += 60 * 10;
				}
				else if (3 == this.changeDigits) {
					trace(`change minute 1s digit\n`);
					if (m1 > 10) {
						m1 = 1;
						nowTime -= 60 * 10;
					}
					else
						nowTime += 60;
				}
				Time.set(nowTime);
				}
			}
			this.timePressed = 0;
		}
		else {
			if ((now - this.lastPressed) < BUTTON_DEBOUNCE_MS)	// ignore if pressed too quickly
				return;
			
			this.timePressed = now;
			this.stillDown = 1;

			this.stillDownTimer = Timer.repeat(id => {
				let elapsed = Time.ticks - this.timePressed;
				if (elapsed > BUTTON_ERASE_PREFS_MS) {
					if (this.blinking) {
						this.blinking = 0;
						clock.display.value(" clr");
					}
				}
				else if (elapsed > BUTTON_ERASE_WARNING_TIME_MS) {
//					trace(`beyond warning time\n`);
					if (4 != this.blinking) {
//						trace(` warning blink\n`);
						this.blinking = 4;
						this.clock.display.blink(100,0xf);
					}
				}
				else if (elapsed > BUTTON_ACCEPT_TIME_MS) {
//					trace(`beyond accept time\n`);
					if (3 != this.blinking) {
//							trace(` slow blink\n`);
						this.blinking = 3;
						this.clock.display.blink(500,0xf);		// slow blink all
					}
				}
				else if (elapsed > BUTTON_CHANGE_TIME_MS) {
//					trace(`beyond change time - switch digits\n`);
					if (!this.blinking) {
						this.changeDigits = 3;					// so release will bring us back to hours
						this.blinking = 1;
						this.clock.display.blink(300,0xc);		// only blink hours
					}
					else if (2 != this.blinking) {
						this.blinking = 2;
						this.clock.display.blink(0,0xf);
					}
				}
			}, 100);
			this.stillDownTimer.monitor = this;

		}
	}
}


let clock = new Clock(prefs);



function checkboxValue(reqVal, item) {
	if (undefined == reqVal[item])
		return 0;
	else
		return parseInt(reqVal[item]);
}

//---------------


function restart() @ "do_restart";

function doRestart(clock) {
	trace("restart\n");
	if (undefined !== clock)
		clock.display.value("bye ");
	restart();
}

