/*
* Copyright (c) 2016-2021 Moddable Tech, Inc.
*
*   This file is part of the Moddable SDK.
*
*   This work is licensed under the
*       Creative Commons Attribution 4.0 International License.
*   To view a copy of this license, visit
*       <http://creativecommons.org/licenses/by/4.0>
*   or send a letter to Creative Commons, PO Box 1866,
*   Mountain View, CA 94042, USA.
*
*/

import Client from "mqtt";
import SecureSocket from "securesocket";
import Timer from "timer";
import Resource from "Resource";

class SecureClient extends Client {
	onReady() {
		this.subscribe("moddable/mqtt/example/#");

		Timer.set(() => {
			this.publish("moddable/mqtt/example/date", Date());
			this.publish("moddable/mqtt/example/random", Math.random());
		 }, 0, 1000);
	};
	onMessage(topic, body) {
		trace(`received "${topic}": ${String.fromArrayBuffer(body)}\n`);
	};
	onClose() {
		trace('lost connection to server\n');
	};
}

new SecureClient({
	host: "test.mosquitto.org",
	timeout: 60_000,
	port: 8883,
	Socket: SecureSocket,
	secure: {
		protocolVersion: 0x0303,
		certificate: new Resource("mosquitto.org.der"),
		trace: false,
	},
});
