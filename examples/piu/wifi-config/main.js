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

import {
	NetworkListScreen
} from "network-list";

import {
	LoginScreen
} from "login";

import {
	ConnectingScreen
} from "connecting";

import {
	ConnectionErrorScreen
} from "connection-error";

import {
	Spinner
} from "spinner";

import WiFi from "wifi";

function getVariantFromSignalLevel(value) {
	let low = -120;
	let high = -40;
	if (value < low) value = low;
	if (value > high) value = high;
	return Math.round(4 * ((value - low) / (high - low)));
}

export default new Application(null, { 
	displayListLength:4096, commandListLength: 4096,
	Behavior: class extends Behavior {
		onCreate(application) {
			this.onSwitchScreen(application, "NETWORK_LIST_SCAN", {})
		}
		doNext(application, nextScreenName, nextScreenData) {
			application.defer("onSwitchScreen", nextScreenName, nextScreenData);
		}
		onSwitchScreen(application, nextScreenName, nextScreenData) {
			if (nextScreenData === undefined) nextScreenData = {};
			if ((application.length) && (nextScreenName !=  "NETWORK_LIST_SCAN")) application.remove(application.first);
			application.purge();
			switch (nextScreenName) {
				case "NETWORK_LIST_SCAN":
					if (!("doNotRemove" in nextScreenData)) {
						if (application.length) application.remove(application.first);
						application.purge();
						application.add(new Spinner());
					}
					// trace("\n\n\n**********\nFree before scan: "+Debug.free()+"\n");
					nextScreenData.networks = null;//[];
					WiFi.scan({}, item => {
					    if (item) {
					    	// trace("Free: "+Debug.free()+"\n");
					    	let strength = getVariantFromSignalLevel(item.rssi);
					    	let ap = { ssid: item.ssid, variant: (item.authentication == "none")? -strength : strength, next: nextScreenData.networks };
					    	nextScreenData.networks = ap;
					    } else {
					    	// trace("Free at end of scan: "+Debug.free()+"\n");
					    	application.delegate("doNext", "NETWORK_LIST", nextScreenData);
					    }
					});
					break;
				case "NETWORK_LIST":
					application.add(new NetworkListScreen(nextScreenData));
					break;
				case "LOGIN":
					application.add(new LoginScreen(nextScreenData));
					break;
				case "CONNECTING":
					application.add(new ConnectingScreen(nextScreenData));
					break;
				case "CONNECTION_ERROR":
					application.add(new ConnectionErrorScreen(nextScreenData));
					break;
			}
		}
	}
});
