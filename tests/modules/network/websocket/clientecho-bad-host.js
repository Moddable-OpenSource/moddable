/*---
description: 
flags: [async, module]
---*/

import {Client} from "websocket"

await $NETWORK.connected;

const ws = new Client({
	host: $NETWORK.invalidDomain,
	path: "/wsChilkatEcho.ashx"
});
ws.callback = function(message, value) {
	if (Client.disconnect === message)
		$DONE();
	else
		$DONE("uunexpected message " + message);
}

$TESTMC.timeout(5_000);
