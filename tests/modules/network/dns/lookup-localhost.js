/*---
description: 
flags: [async, module]
---*/

import Net from "net";

await $NETWORK.connected;

Net.resolve("localhost", (name, address) => {
	if (name !== "localhost") {
		$DONE(`"${name}" should be localhost`);
		return;
	}
	else if (address !== "127.0.0.1")
		$DONE(`"${address}" should be 127.0.1.1`);
	else
		$DONE();
});

$TESTMC.timeout(5_000);
