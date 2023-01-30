/*---
description: 
flags: [async, module]
---*/

import SNTP from "sntp";
import Timer from "timer";

await $NETWORK.connected;

const hosts = ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org", "3.pool.ntp.org"];

let sntp = new SNTP({host: hosts.shift()}, (message, value) => {
	$DONE("callback after close");
});

sntp.close();

Timer.set(() => $DONE(), 2000)
