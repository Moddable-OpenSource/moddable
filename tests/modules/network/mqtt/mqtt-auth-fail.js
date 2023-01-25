/*---
description: 
flags: [async, module]
---*/

import Client from "mqtt";

await $NETWORK.connected;

const mqtt = new Client({
	host: "test.mosquitto.org",
	timeout: 60_000,
	port: 1884,
	user: "rw",
	password: "readwrite-x"
});

mqtt.onReady = function() {
	$DONE("unexpected success");
};

mqtt.onClose = function() {
	$DONE();
};

$TESTMC.timeout(5_000);
