/*---
description: 
flags: [async, module]
---*/

import Net from "net";

await $NETWORK.connected;

Net.resolve($NETWORK.invalidDomain, (name, address) => {
	if (name !== $NETWORK.invalidDomain) {
		$DONE(`"${name}" should be ${$NETWORK.invalidDomain}`);
		return;
	}
	
	if (address)
		$DONE(`"${name}" should not resolve (got "${address}")`);
	else
		$DONE();
});

$TESTMC.timeout(5_000);
