/*---
description: 
flags: [async, module]
---*/

import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(1_000);

assert.throws(Error, () => {new TCP({
		port: 80
	});
});

$DONE();
