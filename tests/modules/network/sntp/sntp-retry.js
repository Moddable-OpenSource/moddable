/*---
description: 
flags: [async, module]
---*/

import SNTP from "sntp";

await $NETWORK.connected;

const host = "marvell.com";		// domain that does not host an NTP server

new SNTP({host}, (message, value) => {
	if (SNTP.time === message) {
		$DONE("got time on domain wihtout NTP server");
	}
	else if (SNTP.retry === message) {
		$DONE();
	}
});

$TESTMC.timeout(10_000);
