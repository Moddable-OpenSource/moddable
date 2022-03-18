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

WiFi.mode = 1;

const mqtt = new Client({
	host: "test.mosquitto.org",
	timeout: 60_000
});
mqtt.subscribe("moddable/mqtt/example/#");

const timer = Timer.repeat(() => {
	mqtt.publish("moddable/mqtt/example/date", Date());
	mqtt.publish("moddable/mqtt/example/random", Math.random());
 }, 1000);
Timer.schedule(timer);

mqtt.onReady = () => Timer.schedule(timer, 0, 1000);
mqtt.onClose = () => Timer.schedule(timer);
mqtt.onMessage = (topic, body) => trace(`received "${topic}": ${String.fromArrayBuffer(body)}\n`);


// calling mqtt.wait is optional, but recommended.
// if wait is not set to false when disconected from a network, the mqttconnection module
//	will still continue to try to reconnect
new WiFi({
		ssid: config.ssid,
		password: config.password
	},
	function (msg) {
		trace(`Wi-Fi ${msg}\n`);
		if (WiFi.gotIP === msg)
			mqtt.wait(false);		// connected
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

