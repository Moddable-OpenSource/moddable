/*
	This example claims the local name "example". It provides an HTTP server that can be
	acccessed at "http://example.local/" from a web browser on any computer or mobile device
 	that supports mDNS.

 	If the name "example" is already claimed on the current network, a unique name with be
 	found by appending "-2" or "-3", etc. to the requested name.

 	The name selected is output to the console. The MDNS constructor provides an optional
	callback to report the selected name to the host application.

*/

import MDNS from "mdns";
import {Server} from "http"
import Net from "net"

let hostName = "example";		// the hostName to claim. updated by callback to actual name claimed.

new MDNS({hostName}, function(message, value) {
	if (1 === message)
		hostName = value;
});

let counter = 0;
let server = new Server({port: 80});
server.callback = function(message, value) {
	if (2 == message)
		this.path = value;

	if (8 == message) {
		let body =	`Client requested path ${this.path}.\n` +
					`Request #${++counter} to this server.\n` +
					`Server host name "${hostName}.local" at address ${Net.get("IP")} on network "${Net.get("SSID")}".\n`;
		return {headers: ["Content-Type", "text/plain"], body};
	}
}
