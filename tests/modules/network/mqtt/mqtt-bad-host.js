/*---
description: 
flags: [async, module]
---*/

import Client from "mqtt";

await $NETWORK.connected;

const mqtt = new Client({
	host: $NETWORK.invalidDomain,
	timeout: 60_000
});

mqtt.onReady = function() {
	$DONE("unexpected success");
};
mqtt.onClose = function() {
	$DONE();
};

$TESTMC.timeout(5_000);
