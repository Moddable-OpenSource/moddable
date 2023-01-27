/*---
description: 
flags: [async, module]
---*/

import {Client} from "websocket"
import Timer from "timer";

await $NETWORK.connected;

const ws = new Client({
	host: "websockets.chilkat.io",
	path: "/wsChilkatEcho.ashx"
});
ws.callback = function(message, value) {
	$DONE("unexpected callback " + message);
}
ws.close();

Timer.set(() => $DONE(), 1000);

$TESTMC.timeout(5_000);
