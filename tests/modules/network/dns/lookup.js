/*---
description: 
flags: [async, module]
---*/

import Net from "net";

await $NETWORK.connected;

let count = 0;

["moddable.tech", "www.moddable.com", "github.com", "tc39.es"].forEach(domain => {
	count++;
	Net.resolve(domain, (name, address) => {
		if (address) {
			count--;
			if (0 === count)
				$DONE();
		}
		else
			$DONE(`could not resolve "${name}"`);
	});
});

$TESTMC.timeout(5_000);
