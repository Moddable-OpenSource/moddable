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

import {Request} from "http"

let request = new Request({	host: "api.service.cloud.kinoma.com",
							path: "/0/sample/hash",
							method: "POST",
							body: JSON.stringify({algorithm: "MD5", string: "Hello, Mark."}),
							response: String
						});

request.callback = function(message, value)
{
	if (5 == message) {
		value = JSON.parse(value);
		trace(`Success: ${value.success}\n`);
		trace(`Hash: ${value.result}\n`);
	}
}
