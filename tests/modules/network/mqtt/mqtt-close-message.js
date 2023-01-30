/*---
description: 
flags: [async, module]
---*/

import Client from "mqtt";
import Timer from "timer";

await $NETWORK.connected;

const topic = `moddable/mqtt/test_${Date.now()}`
const random = Math.random() + Date();

const mqtt = new Client({
	host: "test.mosquitto.org",
	timeout: 60_000
});

mqtt.onReady = function() {
	this.subscribe(`${topic}/#`);
	this.publish(`${topic}/value`, random);
	this.publish(`${topic}/value`, random + "a");
	this.publish(`${topic}/value`, random + "b");
	this.publish(`${topic}/value`, random + "c");
	this.publish(`${topic}/value`, random + "d");
};
mqtt.onMessage = function(aTopic, body) {
	body = String.fromArrayBuffer(body);
	if (!this.closed) {
		if (body === (random + "b")) {
			this.close();
			this.closed = true;
			Timer.set(() => $DONE(), 1000);
		}
	}
	else
		$DONE("unexpected onMessage");

};
mqtt.onClose = function() {
	$DONE("connection failed");
};

$TESTMC.timeout(5_000);
