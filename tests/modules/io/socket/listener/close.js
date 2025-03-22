/*---
description: 
flags: [async, module]
---*/

import Listener from "embedded:io/socket/listener";
import TCP from "embedded:io/socket/tcp";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const port = 8080;
const address = "127.0.0.1";

new Listener({
	port,
	onReadable(count) {
		$DO(() => {
			this.close();
			this.close();

			assert.throws(SyntaxError, () => this.read());
		})();
	}
});

new TCP({
	address,
	port
});
