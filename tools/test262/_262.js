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

globalThis.$MAIN = function() @ "_xsbug_main";
globalThis["<xsbug:import>"] = function(script, path, line) @ "_xsbug_import_";
globalThis["<xsbug:module>"] = function(script, path, line) @ "_xsbug_module_";
globalThis["<xsbug:script>"] = function(script, path, line) @ "_xsbug_script_";

globalThis.$262 = {
	agent: {
		get safeBroadcast() @ "fx_agent_get_safeBroadcast",
		set safeBroadcast(it) @ "fx_agent_set_safeBroadcast",
		broadcast() @ "_262_agent_broadcast",
		getReport() @ "_262_agent_getReport",
		monotonicNow() @ "_262_agent_monotonicNow",
		sleep() @ "_262_agent_sleep",
		start() @ "_262_agent_start",
		stop() @ "_262_agent_stop",
	},
	createRealm() @ "_262_createRealm",
	detachArrayBuffer() @ "_262_detachArrayBuffer",
	gc() @ "_262_gc",
	evalScript() @ "_262_evalScript",
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
