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
import SecureSocket from "securesocket";

let request = new Request({host: "www.example.com", path: "/", response: String,
		port: 443, Socket: SecureSocket});
request.callback = function(message, value, etc)
{
	if (Request.header === message)
		trace(`${value}: ${etc}\n`);
	else if (Request.responseComplete === message)
		trace(value + "\n");
}
