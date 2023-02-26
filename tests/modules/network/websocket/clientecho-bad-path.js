/*---
description: 
flags: [async, module]
---*/

import {Client} from "websocket"

await $NETWORK.connected;

const ws = new Client({
	host: "websockets.chilkat.io",
	path: "/wsChilkatEcho.ashx_BAD"
});
ws.callback = function(message, value) {
	if (Client.connect === message)
		;
	else if (Client.disconnect === message)
		$DONE();
	else
		$DONE("uunexpected message " + message);
}

$TESTMC.timeout(5_000);
