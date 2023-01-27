/*---
description: 
flags: [async, module]
---*/

import {Client} from "websocket"
import Timer from "timer"

await $NETWORK.connected;

const ws = new Client({
	host: "websockets.chilkat.io",
	path: "/wsChilkatEcho.ashx"
});
ws.callback = function(message, value) {
	switch (message) {
		case Client.connect:
			break;

		case Client.handshake:
			this.write("01234567");
			this.write("89abcdef");
			this.write("01234567");
			this.write("89abcdef");
			this.write("01234567");
			this.write("89abcdef");
			break;

		case Client.receive:
			this.close();
			Timer.set(() => $DONE(), 1000);
			break;

		default:
			$DONE("unexpected callback " + message);
			break;
	}
}

$TESTMC.timeout(5_000);
