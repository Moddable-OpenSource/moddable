/*---
description: 
flags: [async, module]
---*/

import {Client} from "websocket"

await $NETWORK.connected;

const random = Math.random() + " @ " + Date();
const ws = new Client({
	host: "websockets.chilkat.io",
	path: "/wsChilkatEcho.ashx"
});
ws.callback = function(message, value) {
	switch (message) {
		case Client.connect:
			break;

		case Client.handshake:
			this.write(JSON.stringify({random}));
			break;

		case Client.receive:
			value = JSON.parse(value);
			if (random === value.random)
				$DONE();
			else
				$DONE(`expected ${random}; got ${value.random}`);
			break;

		case Client.disconnect:
			$DONE(`websocket close`);
			break;
	}
}

$TESTMC.timeout(5_000);
