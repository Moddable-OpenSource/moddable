/*---
description: 
flags: [async, module]
---*/

import Ping from "ping";
import Timer from "timer";

await $NETWORK.connected;

const host = "test.moddable.com";
const id = 0;
const interval = 500;

let ping = new Ping({host, id, interval}, function(message, value, etc) {
	if (!ping)
		$DONE("callback after close");
});

Timer.set(() => {
	ping.close();
	ping = null;
}, 600);

Timer.set(() => $DONE(), 2500);

$TESTMC.timeout(5_000);
