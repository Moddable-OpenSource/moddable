/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
	createConnection,
	createLongLivedTokenAuth,
	ERR_INVALID_AUTH
} from "home-assistant-js-websocket";

const ACCESS_TOKEN = "** REPLACE WITH YOUR LONG-LIVED ACCESS TOKEN **";

let connection;
try {
	connection = await createConnection({
			auth: createLongLivedTokenAuth("http://homeassistant.local:8123", ACCESS_TOKEN)
		}
	);

	const devices = await connection.sendMessagePromise({
		type: "config/device_registry/list",
	});

	for (const device of devices) {
		console.log("-----");
		console.log(`Name: ${device.name || device.name_by_user || "(unnamed)"}`);
		console.log(`ID: ${device.id}`);
		console.log(`Manufacturer: ${device.manufacturer ?? ""}`);
		console.log(`Model: ${device.model ?? ""}`);
		console.log(`Area ID: ${device.area_id ?? ""}`);
	}
}
catch (e) {
	if (e === ERR_INVALID_AUTH)
		trace(`Home Assistant authentication failed. Check ACCESS_TOKEN: ${ACCESS_TOKEN}.\n`);
	else if ("number" === typeof e)
		trace(`Home Assistant error code ${e}.\n`);
	else
		trace(`startup failed: ${e.message}\n`);
}
connection?.close();
