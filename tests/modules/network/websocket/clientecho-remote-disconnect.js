/*---
description: 
flags: [async, module]
---*/

import {Client} from "websocket"

await $NETWORK.connected;

// note: this server disconnects after 16 messages
const ws = new Client({
	host: "websockets.chilkat.io",
	path: "/wsChilkatEcho.ashx"
});
ws.callback = function(message, value) {
	switch (message) {
		case Client.connect:
			this.counter = 0;
			break;

		case Client.handshake:
			this.write(this.counter.toString());
			this.counter += 1;
			break;

		case Client.receive:
			if (!this.expectDisconnect) {
				this.write(this.counter.toString());
				this.counter += 1;
				this.expectDisconnect = this.counter >= 16;
			}
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
