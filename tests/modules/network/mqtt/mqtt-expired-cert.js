/*---
description: 
flags: [async, module]
---*/

import Client from "mqtt";
import SecureSocket from "securesocket";
import Resource from "Resource";

await $NETWORK.connected;

const mqtt = new Client({
	host: "test.mosquitto.org",
	timeout: 60_000,
	port: 8887,
	Socket: SecureSocket,
	secure: {
		protocolVersion: 0x0303,
		certificate: new Resource("mosquitto.org.der")
	},
});

mqtt.onReady = function() {
	$DONE("unexpected success");
};
mqtt.onClose = function() {
	$DONE();
};

$TESTMC.timeout(5_000);
