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

import Client from "mqtt/connection";
import Net from "net";
import Timer from "timer";
import WiFi from "wifi/connection";
import config from "mc/config";

const mqtt = new Client({
	host: "test.mosquitto.org"
});

mqtt.onReady = function() {
	this.subscribe("moddable/mqtt/example/#");

	this.timer = Timer.repeat(() => {
		this.publish("moddable/mqtt/example/date", (new Date()).toString());
		this.publish("moddable/mqtt/example/random", Math.random());
	 }, 1000);
};
mqtt.onMessage = function(topic, body) {
	if (body.byteLength > 128)
		trace(`received "${topic}": ${body.byteLength} bytes\n`);
	else
		trace(`received "${topic}": ${String.fromArrayBuffer(body)}\n`);
};
mqtt.onClose = function() {
	trace('lost connection to server\n');
	if (this.timer) {
		Timer.clear(this.timer);
		delete this.timer;
	}
};



new WiFi(
	{
		ssid: config.ssid,
		password: config.password
	},
	function (msg) {
		trace(`Wi-Fi ${msg}\n`);
		if (WiFi.gotIP === msg) {
			mqtt.id = "moddable_" + Net.get("MAC");
			mqtt.wait(false);		// connected
		}
		else if (WiFi.disconnected === msg)
			mqtt.wait(true);		// lost connection
	}
);
mqtt.wait(true);	// not connected yet

// enable this on ESP8266 and ESP32 to test Wi-Fi disconnect handling
if (0) {
	Timer.repeat(() => {
		WiFi.disconnect();
	}, 15_000);
}
