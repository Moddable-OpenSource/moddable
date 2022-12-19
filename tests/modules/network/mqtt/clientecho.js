/*---
description: 
flags: [async, module]
---*/

import Client from "mqtt";

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
};
mqtt.onMessage = function(aTopic, body) {
	$DO(() => {
		assert.sameValue(aTopic, `${topic}/value`);
		assert.sameValue(random, String.fromArrayBuffer(body));
	})();
};
mqtt.onClose = function() {
	$DONE("connection failed");
};

$TESTMC.timeout(5_000);
