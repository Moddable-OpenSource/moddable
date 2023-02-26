/*---
description: 
flags: [async, module]
---*/

import Net from "net";

await $NETWORK.connected;

const lookupAddress = "1.2.3.4"; 

Net.resolve(lookupAddress, (name, address) => {
	if (name !== lookupAddress) {
		$DONE(`"${name}" should be ${lookupAddress}`);
		return;
	}
	else if (address !== lookupAddress)
		$DONE(`"${address}" should be ${lookupAddress}`);
	else
		$DONE();
});

$TESTMC.timeout(5_000);
