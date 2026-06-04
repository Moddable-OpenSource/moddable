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
	ERR_INVALID_AUTH,
	type Connection,
} from "home-assistant-js-websocket";
import Timer from "timer";

// Compact diff format used by the "subscribe_entities" command.
// See home-assistant/core: components/websocket_api/const.py (COMPRESSED_STATE_*)
type CompressedState = {
	s: string;
	a: Record<string, unknown>;
	c: string | { id: string; parent_id: string | null; user_id: string | null };
	lc: number;
	lu?: number;
};

type CompressedDiff = {
	"+"?: Partial<CompressedState>;
	"-"?: { a?: string[] };
};

type EntitiesEvent = {
	a?: Record<string, CompressedState>;   // added (full entity)
	r?: string[];                          // removed entity ids
	c?: Record<string, CompressedDiff>;    // changed (per-entity diff)
};

// Friendly name ("Rey Skywalker") or entity_id ("light.rey_skywalker"). entity_id preferred.
const LIGHT = "light.rey_skywalker";
const INTERVAL = 30_000;		// milliseconds
const ACCESS_TOKEN = "** REPLACE WITH YOUR LONG-LIVED ACCESS TOKEN **"; 

async function findLight(connection: Connection, query: string): Promise<string> {
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

let connection: Connection | undefined;
try {
	connection = await createConnection({
			auth: createLongLivedTokenAuth("http://homeassistant.local:8123", ACCESS_TOKEN)
		}
	);

	const lightID = await findLight(connection, LIGHT);
	let state: string | undefined;

	await connection.subscribeMessage<EntitiesEvent>(msg => {
		const added = msg.a?.[lightID]?.s;
		const changed = msg.c?.[lightID]?.["+"]?.s;
		let next: string | undefined;
		if (added !== undefined)
			next = added;
		else if (changed !== undefined)
			next = changed;
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
	if (e === ERR_INVALID_AUTH)
		trace(`Home Assistant authentication failed. Check ACCESS_TOKEN: ${ACCESS_TOKEN}.\n`);
	else if ("number" === typeof e)
		trace(`Home Assistant error code ${e}.\n`);
	else
		trace(`startup failed: ${(e as Error).message}\n`);
	connection?.close();
}
