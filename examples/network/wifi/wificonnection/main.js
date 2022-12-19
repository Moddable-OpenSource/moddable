/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
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

import WiFi from "wifi/connection";

WiFi.mode = 1; // station mode

if (1) {
	// single access point
	new WiFi(
		{
			ssid: "access point name",
			password: "password"
		},
		function (msg) {
			trace(`Wi-Fi ${msg}\n`);
		}
	);
}
else {
	// multiple access points

	const AccessPoints = [
		{
			ssid: "access point name one",
			password: "invalid!"		// force failure in simulator
		},
		{
			ssid: "access point name two",
			password: "password"
		},
		{
			ssid: "access point name three",
			password: "password"
		}
	];

	new WiFi(
		{},
		function (msg, value) {
			trace(`Wi-Fi ${msg}`, (undefined !== value) ? ` @ ${value}` : "", "\n");
			if ("getAP" === msg)
				return AccessPoints[value % AccessPoints.length];
		}
	);
}
