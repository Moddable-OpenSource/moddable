/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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

trace("*** config properties ***\n");
let keys = Object.keys(config);
keys.forEach(key => trace(`key: ${key}, value: ${config[key]}\n`));
trace("\n");

if (config.sntp)
	trace(`SNTP host: ${config.sntp}\n`);

async function loadModule(specified) {
	const Echo = (await import(specified)).default;
	Echo.print("Fini!");
}

loadModule(config.echoModuleSpecifier);
