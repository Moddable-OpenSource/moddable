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

import listen from "listen";
import { Headers, Response } from "listen";

async function main() {
	for await (const connection of listen({ port: 80 })) {
		const request = connection.request;
		trace(`${request.method}\n`);
		trace(`${request.url}\n`);
		for (const [header, value] of request.headers)
			trace(`${header}: ${value}\n`);		
		const body = await request.text();
		trace(`${body}\n`);		
		connection.respondWith(new Response("1 2 3 4 5 6 7 8\n", { status: 200 }));		
	}
}

main();