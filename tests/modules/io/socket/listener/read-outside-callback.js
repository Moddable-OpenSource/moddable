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

const {promise: p, resolve, reject} = Promise.withResolvers();

const l = new Listener({
	port,
	onReadable(count) {
		resolve(count);
	}
});

new TCP({
	address,
	port
});

assert.sameValue(await p, 1, "number of incoming rqeuests");

const tcp = l.read();
if (!(tcp instanceof TCP))
	$DONE("read should return TCP instance");
if (tcp.remoteAddress !== address)
	$DONE("expected remoteAddress " + address);
if (tcp.format !== "buffer")
	$DONE("expected format of buffer");

assert.sameValue(l.read(), undefined, "read returns undefined when no socket pending");

$DONE();
