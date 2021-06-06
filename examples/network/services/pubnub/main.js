/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import {PubNub} from "pubnub";

let pubnub = new PubNub({
	publishKey:"demo",
	subscribeKey:"demo"
});

let channel = "hello_world";
let message = "Hello World";

function publishSampleMessage() {
	trace("Publish sample message: " + message + "\n");
	pubnub.publish({ channel, message }, (error, data) => {
		trace(error + " " + !!data + "\n");
	}, this);
}

pubnub.addListener({
	message(event) {
		trace(JSON.stringify(event.message) + "\n");
	},
	status(event) {
		if (event.category === "PNConnectedCategory") {
			publishSampleMessage();
		}
	}
});

trace("Subscribing...\n");
pubnub.subscribe({ channels:[ channel ] });
