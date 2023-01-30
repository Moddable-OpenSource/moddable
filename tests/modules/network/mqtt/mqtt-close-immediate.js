/*---
description: 
flags: [async, module]
---*/

import Client from "mqtt";
import Timer from "timer";

await $NETWORK.connected;

const mqtt = new Client({
	host: "test.mosquitto.org",
	timeout: 60_000
});

mqtt.onReady = function() {
	$DONE("unexpected ready");
};
mqtt.onClose = function() {
	$DONE("unexpected close");
};
mqtt.close();

Timer.set(() => $DONE(), 1000);

$TESTMC.timeout(5_000);
