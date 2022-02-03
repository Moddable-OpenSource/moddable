/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
 * Copyright (c) Wilberforce
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

import { Bridge, HTTPServer } from "bridge/webserver";

// Single Page application. Map page requests back to /

export class BridgeRewriteSPA extends Bridge {
	#routes
	constructor(routes) {
		super();
		this.#routes=routes
	}

	handler(req, message, value, etc) {
		
		switch (message) {
			case HTTPServer.status:
				if ( this.#routes.includes(value) ) { // To do: possibly use Set?
					trace(`Rewrite: ${value}\n`);
					value='/';
					trace(`Rewrite now: ${value}\n`);
				}
				break;
		}
		return this.next?.handler(req, message, value, etc);
	}
}
