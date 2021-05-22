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

import Preference from "preference";

const domain = "test";

Preference.set(domain, "boolean", true);
Preference.set(domain, "integer", 12);
Preference.set(domain, "string", "this is a test");
Preference.set(domain, "arraybuffer", Uint8Array.from([1, 2, 3, 4, 5]).buffer);
Preference.set(domain, "spaces in name", "OK!");

trace(Preference.get(domain, "boolean") + "\n");
trace(Preference.get(domain, "integer") + "\n");
trace(Preference.get(domain, "string") + "\n");
let buffer = Preference.get(domain, "arraybuffer");
if (undefined !== buffer)
	trace((new Uint8Array(buffer)).join(", ") + "\n");
else
	trace("(not found)\n");
trace(Preference.get(domain, "spaces in name") + "\n");

Preference.delete(domain, "boolean");
Preference.delete(domain, "integer");
Preference.delete(domain, "string");
Preference.delete(domain, "arraybuffer");
Preference.delete(domain, "spaces in name");

trace("done\n");
