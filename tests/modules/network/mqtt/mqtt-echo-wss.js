/*---
description: 
flags: [async, module]
---*/

import Client from "mqtt";
import SecureSocket from "securesocket";
import Resource from "Resource";

await $NETWORK.connected;

const topic = `moddable/mqtt/test_${Date.now()}`
const random = Math.random() + Date();

const mqtt = new Client({
	host: "test.mosquitto.org",
	port: 8081,
	timeout: 60_000,
	path: "/",
	Socket: SecureSocket,
	secure: {
		protocolVersion: 0x0303
	},
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
