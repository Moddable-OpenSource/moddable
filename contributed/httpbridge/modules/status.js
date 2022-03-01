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

export class BridgeStatus extends Bridge {
	#status;
	constructor(status=404) {
		super();
		this.#status=status
	}
	handler(req, message, value, etc) {
		switch (message) {

			case HTTPServer.prepareResponse:
				return {
					status: this.#status
				};
		}
	}
}
