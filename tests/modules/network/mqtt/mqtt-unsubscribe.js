/*---
description: 
flags: [async, module]
---*/

import Client from "mqtt";

await $NETWORK.connected;

const topic = `moddable/mqtt/test_${Date.now()}`

const mqtt = new Client({
	host: "test.mosquitto.org",
	timeout: 60_000
});

mqtt.onReady = function() {
	this.subscribe(`${topic}/a`);
	this.subscribe(`${topic}/b`);
	this.publish(`${topic}/a`, "1");
};
mqtt.onMessage = function(aTopic, body) {
	body = String.fromArrayBuffer(body);

	if (aTopic === `${topic}/a`) {
		if ("1" !== body)
			$DONE("unexpected message on " + aTopic);
		this.unsubscribe(`${topic}/a`);
		this.publish(`${topic}/a`, "2");
		this.publish(`${topic}/b`, "3");
	}
	else if (aTopic === `${topic}/b`) {
		if ("3" === body)
			$DONE();
		else
			$DONE("unexpected message on " + aTopic);
	}
};
mqtt.onClose = function() {
	$DONE("connection failed");
};

$TESTMC.timeout(5_000);
