/*
 * Copyright (c) 2018-2021  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
 
import Timer from "timer";

globalThis.$MAIN = function() { return native("_xsbug_main").call(this); };
globalThis["<xsbug:import>"] = function(script, path, line) { return native("_xsbug_import_").call(this, script, path, line); };
globalThis["<xsbug:module>"] = function(script, path, line) { return native("_xsbug_module_").call(this, script, path, line); };
globalThis["<xsbug:script>"] = function(script, path, line) { return native("_xsbug_script_").call(this, script, path, line); };

globalThis.$262 = {
	agent: {
		get safeBroadcast() { return native("fx_agent_get_safeBroadcast").call(this); },
		set safeBroadcast(it) { native("fx_agent_set_safeBroadcast").call(this, it); },
		broadcast() { return native("_262_agent_broadcast").call(this); },
		getReport() { return native("_262_agent_getReport").call(this); },
		monotonicNow() { return native("_262_agent_monotonicNow").call(this); },
		sleep() { return native("_262_agent_sleep").call(this); },
		start() { return native("_262_agent_start").call(this); },
		stop() { return native("_262_agent_stop").call(this); },
	},
	createRealm() { return native("_262_createRealm").call(this); },
	detachArrayBuffer() { return native("_262_detachArrayBuffer").call(this); },
	gc() { return native("_262_gc").call(this); },
	evalScript() { return native("_262_evalScript").call(this); },
	global: globalThis,
}
globalThis.clearInterval = Timer.clear;
globalThis.clearTimeout = Timer.clear;
globalThis.print = function() {
	let c = arguments.length;
	for (let i = 0; i < c; i++) {
		if (i)
			trace(" ");
		trace(arguments[i]);
	}
	trace("\n");
}
globalThis.setInterval = Timer.repeat;
globalThis.setTimeout = Timer.set;
