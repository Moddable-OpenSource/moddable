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
	callService,
	getStates,
} from "home-assistant-js-websocket";
import Timer from "timer";

// Friendly name ("Rey Skywalker") or entity_id ("light.rey_skywalker"). entity_id preferred.
const LIGHT = "light.rey_skywalker";
const INTERVAL = 30_000;		// milliseconds
const ACCESS_TOKEN = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJmMzhkNGQxM2EzZWE0YTE3YmM5NDcyZWU3MDlhZjkwMSIsImlhdCI6MTc3OTgzMzY4NiwiZXhwIjoyMDk1MTkzNjg2fQ.2KGZ70IjOmVDUVIE5Hrp_AQg1ud5f7DzzfeKMDJGY9w"; 

const connection = await createConnection({
		auth: createLongLivedTokenAuth("http://homeassistant.local:8123", ACCESS_TOKEN)
	}
);

async function findLight(connection, query) {
	if (query.startsWith("light."))
		return query;
	const states = await getStates(connection);

	query = query.toLowerCase();
	const match = states.find(state =>
		(state.entity_id.startsWith("light.")) &&
		(state.attributes.friendly_name?.toLowerCase() === query)
	);
	if (!match)
		throw new Error(`light "${query}" not found`);
	return match.entity_id;
}

try {
	const lightID = await findLight(connection, LIGHT);
	let state;

	await connection.subscribeMessage(msg => {
		let next;
		if (msg.a?.[lightID])
			next = msg.a[lightID].s;
		else if (msg.c?.[lightID]?.["+"]?.s !== undefined)
			next = msg.c[lightID]["+"].s;
		else {
			if (msg.a && (undefined === state)) {
				trace(`entity "${lightID}" not found\n`);
				connection.close();
			}
			return;
		}
		if (next !== state) {
			state = next;
			trace(`${lightID} -> ${state}\n`);
		}
	}, {type: "subscribe_entities", entity_ids: [lightID]});

	Timer.set(() => {
		if (("on" !== state) && ("off" !== state))
			return;

		const service = ("on" === state) ? "turn_off" : "turn_on";
		trace(`toggling ${lightID}: ${service}\n`);
		callService(connection, "light", service, undefined, {entity_id: lightID});
	}, 0, INTERVAL);
}
catch (e) {
	trace(`startup failed: ${e.message}\n`);
	connection.close();
}
