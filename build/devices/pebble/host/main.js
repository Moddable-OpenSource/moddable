/*
 * Copyright (c) 2025-2026  Moddable Tech, Inc.
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
import Resource from "Resource"
import Global from "pebble/global"
import ArchiveResource from "pebble/archive-resource";
import ArchiveCompartment from "ArchiveCompartment"
import keyValue from "embedded:storage/key-value";
import WebStorage from "webstorage";
import {} from "piu/MC"
import Instrumentation from "instrumentation";
import Debug from "debug";

import HTTPClient from "embedded:network/http/client";
import WebSocketClient from "embedded:network/websocket/client";
import { fetch } from "fetch";
import Headers from "headers";
import { URL, URLSearchParams } from "url";
import WebSocket from "WebSocket";

const clearImmediate = Timer.clear;
const setImmediate = function(callback) { return Timer.set(callback) };
const setInterval = function(callback, delay) { return Timer.repeat(callback, delay) };
const setTimeout = function(callback, delay) { return Timer.set(callback, delay) };

const console = Object.freeze({
	log: function (...args) {
		trace(...args, "\n");
	}
});

class AppInfo {
	static get uuid() { return native("xs_appinfo_get_uuid").call(this); }
	static get name() { return native("xs_appinfo_get_name").call(this); }
	static get isWatchface() { return native("xs_appinfo_get_isWatchface").call(this); }
}

const state = {};		// for mutable runtime state

const blockedWatchFace = Object.freeze([
	"pebble/button"
]);

globalThis.device = Object.freeze({
	network: {
		http: {
			io: HTTPClient,
			protocol: "http"			
		},
		https: {
			io: HTTPClient,
			protocol: "https"			
		},
		ws: {
			io: WebSocketClient,
			secure: false		// cannot use protocol as property name
		},
		wss: {
			io: WebSocketClient,
			secure: true
		}
	},
	get files() {
		state.files ??= state.mod.importNow("embedded:storage/files").default;
		return state.files;
	},
	keyValue,
	info: {
		get serialNumber() {return native("xs_device_serialNumber_get").call(this);},
		get language() {return native("xs_device_language_get").call(this);}
	}
}, true);

const hook = function(specifier) {
	if (AppInfo.isWatchface && blockedWatchFace.includes(specifier))
		throw new Error(specifier + " blocked in watchface");

	return {namespace: specifier};		// map through host modules
};

export default function() {
	state.archive = (new ArchiveResource())?.archive;
	console.log(`Found mod "${state.archive.name}"`);

	globalThis.watch = new Global;		// so modules (like global) can reach the instance

	const globals = {
		console,
		clearImmediate,
		clearTimeout: clearImmediate,
		clearInterval: clearImmediate,
		setImmediate,
		setInterval,
		setTimeout,
		screen,
		Date,
		Math,
		Resource,
		watch: globalThis.watch,

		// network
		device,
		fetch,
		Headers,
		URL,
		URLSearchParams,
		WebSocket,

		// Piu
		Application,
		Behavior,
		Column,
		Container,
		Content,
		Label,
		Layout,
		Link,
		Locals,
		Port,
		Row,
		Scroller, 
		Skin,
		Style,
		Text,
		Texture,
		Transition,
			
		blendColors, hsl, hsla, rgb, rgba, 
		
		Inverter,
		QRCode,
		RoundRect,
		SVGImage,
		ScreenBuffer,
		
		importNow(specifier) {
			return state.mod.importNow(specifier);
		},
		get localStorage() {
			state.localStorage ??= new WebStorage(keyValue.open({path: `local-${AppInfo.uuid}`}));
			return state.localStorage;
		}
	};

	state.mod = new ArchiveCompartment(state.archive, {
		globals,
		modules: {},
		loadNowHook: hook,
		loadHook: hook
	});

	Timer.set(async () => {
		await state.mod.import("main");
	});
}
