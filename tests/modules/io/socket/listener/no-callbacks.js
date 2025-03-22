/*---
description: 
flags: [async, module]
---*/

import Listener from "embedded:io/socket/listener";
import TCP from "embedded:io/socket/tcp";
import Timer from "timer";

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const port = 8080;
const address = "127.0.0.1";

const l = new Listener({
	port,
});

assert.sameValue(l.read(), undefined, "no incoming requests initially");

new TCP({
	address,
	port
});

Timer.repeat(id => {
	let tcp;
	try {
		tcp = l.read();
		if (undefined === tcp)
			return;
		l.close();
		Timer.clear(id);
	}
	catch (e) {
		$DONE(e);
		return;
	}

	$DO(() => {
		if (!(tcp instanceof TCP))
			$DONE("read should return TCP instance");
		if (tcp.remoteAddress !== address)
			$DONE("expected remoteAddress " + address);
		if (tcp.format !== "buffer")
			$DONE("expected format of buffer");
	})();
}, 50);
