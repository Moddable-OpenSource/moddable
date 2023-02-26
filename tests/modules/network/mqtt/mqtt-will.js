/*---
description: 
flags: [async, module]
---*/

import Client from "mqtt";
import Timer from "timer";

await $NETWORK.connected;

const topic = `moddable/mqtt/will_${Date.now()}`;
const willMsg = "disconnected!";

const mqtt = new Client({
	host: "test.mosquitto.org",
	timeout: 60_000,
	will: {
		topic,
		message: willMsg,
		retain: false,
		quality: 0
	}
});

mqtt.onReady = function() {
	const verifier = new Client({
		host: "test.mosquitto.org",
		timeout: 60_000
	});
	verifier.onReady = function() {
		this.subscribe(topic);
		Timer.set(() => mqtt.close(true), 1000);
	}
	verifier.onMessage = function(aTopic, msg) {
		msg = String.fromArrayBuffer(msg);
		if ((willMsg === msg) && (topic === aTopic))
			$DONE();
		else
			$DONE("unexpected will " + topic + "/" + msg);
	};
	verifier.onClose = function() {
		$DONE("verifier connection failed");
	};


};
mqtt.onClose = function() {
	$DONE("connection failed");
};

$TESTMC.timeout(5_000);
