/*
	This example claims the local name "example". It provides an HTTP server that can be
	acccessed "http://example.local/" from a web browser on any computer or phone
 	that supports mDNS.

 	If the name "example" is already claimed on the current network, a unique name with be
 	found by adding a "-2" or "-3", etc. to the end of the requested name.

 	The name selected is output to the console. The MDNS class supports an optional
	callback to report the selected name to the host application.

*/

import MDNS from "mdns";
import {Server} from "http"

new MDNS({hostName: "example"});

let counter = 0;
let server = new Server({port: 80});
server.callback = function(message, value) {
	if (2 == message)
		this.path = value;

	if (8 == message)
		return {headers: ["Content-type", "text/plain"],
				body: `Hello, client requesting path ${this.path}.\nThis is request #${++counter} to this server.`};
}
