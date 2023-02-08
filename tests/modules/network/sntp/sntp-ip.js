/*---
description: 
flags: [async, module]
---*/

import SNTP from "sntp";
import Net from "net";

await $NETWORK.connected;

const hosts = ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org", "3.pool.ntp.org"];
const addresses = [];

hosts.forEach(domain => {
	Net.resolve(domain, (name, address) => {
		if (address) {
			addresses.push(address);
			if (addresses.length === hosts.length) {
				new SNTP({host: addresses.shift()}, (message, value) => {
					if (SNTP.time === message)
						$DONE();
					else if (SNTP.error === message) {
						if (addresses.length)
							return addresses.shift();
						$DONE("failed " + value);
					}
				});
			}
		}
		else
			$DONE(`could not resolve "${name}"`);
	});
});

$TESTMC.timeout(5_000);
