/*---
description: 
flags: [async, module]
---*/

import SNTP from "sntp";

await $NETWORK.connected;

const hosts = [$NETWORK.invalidDomain, "0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org", "3.pool.ntp.org"];

let didFailover = false;

new SNTP({host: hosts.shift()}, (message, value) => {
	if (SNTP.time === message) {
		if (didFailover)
			$DONE();
		else
			$DONE("got time without failover");
	}
	else if (SNTP.error === message) {
		if (hosts.length) {
			didFailover = true;
			return hosts.shift();
		}
		$DONE("failed " + value);
	}
});

$TESTMC.timeout(5_000);
