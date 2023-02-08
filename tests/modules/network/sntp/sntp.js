/*---
description: 
flags: [async, module]
---*/

import SNTP from "sntp";

await $NETWORK.connected;

const hosts = ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org", "3.pool.ntp.org"];

new SNTP({host: hosts.shift()}, (message, value) => {
	if (SNTP.time === message)
		$DONE();
	else if (SNTP.error === message) {
		if (hosts.length)
			return hosts.shift();
		$DONE("failed " + value);
	}
});
