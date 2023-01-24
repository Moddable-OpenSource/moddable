/*---
description: 
flags: [async, module]
---*/

import Timer from "timer";
import {Request} from "http";

await $NETWORK.connected;

const request = new Request({host: "www.example.com", path: "/"});
request.callback = function(message, value, etc) {
	$DONE("unexpected callback");
}
request.close();
Timer.set(() => $DONE(), 1000)

$TESTMC.timeout(2_000);
