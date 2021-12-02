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

import Client from "mqtt";
import Timer from "timer";

const mqtt  = new Client({
	host: "test.mosquitto.org",
	timeout: 60_000
});

mqtt.onReady = function() {
	this.subscribe("moddable/mqtt/example/#");

	this.timer = Timer.set(() => {
		this.publish("moddable/mqtt/example/date", Date());
		this.publish("moddable/mqtt/example/random", Math.random());
	 }, 0, 1000);
};
mqtt.onMessage = function(topic, body) {
	trace(`received "${topic}": ${String.fromArrayBuffer(body)}\n`);
};
mqtt.onClose = function() {
	trace('lost connection to server\n');
	if (this.timer) {
		Timer.clear(this.timer);
		delete this.timer;
	}
};
