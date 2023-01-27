/*---
description: 
flags: [async, module]
---*/

import {Client} from "websocket"
import Timer from "timer"

await $NETWORK.connected;

// note: this server disconnects after 16 messages
const ws = new Client({
	host: "websockets.chilkat.io",
	path: "/wsChilkatEcho.ashx"
});
ws.callback = function(message, value) {
	switch (message) {
		case Client.connect:
			break;

		case Client.handshake:
			for (let i = 0; i < 10; i++)
				this.write(i.toString());
			break;

		case Client.receive:
			for (let i = 10; i < 20; i++)
				this.write(i.toString());
			this.expectDisconnect = true;
			break;

		case Client.disconnect:
			if (this.expectDisconnect)
				$DONE();
			else
				$DONE("unexpected disconnect");
			break;
	}
}

$TESTMC.timeout(5_000);
