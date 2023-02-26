/*---
description: 
flags: [async, module]
---*/

import Ping from "ping";
import Timer from "timer";

await $NETWORK.connected;

const host = "test.moddable.com";
const id = 0;
const interval = 250;

let ping = new Ping({host, id, interval}, function(message, value, etc) {
	if (!ping)
		$DONE("callback after close");
});

ping.close();
ping = null;

Timer.set(() => $DONE(), 1000);

$TESTMC.timeout(5_000);
