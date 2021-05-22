/*
* Copyright (c) 2016-2020 Moddable Tech, Inc.
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
import Net from "net";
import Timer from "timer";

let mqtt  = new Client({
	host: "test.mosquitto.org",
	id: "moddable_" + Net.get("MAC"),
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
