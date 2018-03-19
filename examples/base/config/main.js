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
/*
 *     Additional properties can be defined on the command line:
 *     mcconfig -d -m name=Brian city="Del Mar"
 */

import config from "mc/config";

let keys = Object.keys(config);

trace("*** config properties ***\n");
keys.forEach((key, index) => {
	trace(`key: ${key}, value: ${config[key]}\n`);
});
trace("\n");

if (config.sntp)
	trace(`NTP host: ${config.sntp}\n`);
	
let echo = require(config.echo);
echo.print("Fini!");
	
