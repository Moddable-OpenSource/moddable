/*
 * Copyright (c) 2025  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import Timer from "timer"
import ArchiveResource from "pebble/archive-resource";
import ArchiveCompartment from "ArchiveCompartment"

import HTTPClient from "embedded:network/http/client";
import { fetch } from "fetch";
import { URL, URLSearchParams } from "url";

globalThis.device = Object.freeze({
	network: {
		http: {
			io: HTTPClient,
			protocol: "http"			
		},
		https: {
			io: HTTPClient,
			protocol: "https"			
		}
	}
}, true);


const clearImmediate = function(id) { return Timer.clear(id) };
const setImmediate = function(callback) { return Timer.set(callback) };
const setInterval = function(callback, delay) { return Timer.repeat(callback, delay) };
const setTimeout = function(callback, delay) { return Timer.set(callback, delay) };

const console = Object.freeze({
	log(...args) {
		trace(...args);
	}
});

class AppInfo {
	static get uuid() { return native("xs_appinfo_get_uuid").call(this); }
	static get name() { return native("xs_appinfo_get_name").call(this); }
	static get isWatchface() { return native("xs_appinfo_get_isWatchface").call(this); }
}

export default function() {
	try {
		const r = new ArchiveResource(0);		// mod is in resource 0 in example. make this configurable.
		const archive = r.archive;
		console.log(`Found mod "${archive.name}"`);

		const globals = {
			console,
			clearImmediate,
			clearTimeout: clearImmediate,
			clearInterval: clearImmediate,
			setImmediate,
			setInterval,
			setTimeout,
			Date,
			Math,
			device,
			fetch,
			URL,
			URLSearchParams
		};

		const mod = new ArchiveCompartment(archive, {
			globals,
			modules: {},
			loadNowHook(specifier) {
				return {namespace: specifier};		// map through host modules
			}
		});
		mod.importNow("main");
	}
	catch (e) {
		console.log(`Error loading mod in app "${AppInfo.name}": ${e}`);
	}
}

// force into the symbol table
let target
