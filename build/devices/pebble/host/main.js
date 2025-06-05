import Timer from "timer"
import {Rocky} from "pebble/graphics"
import ArchiveResource from "pebble/archive-resource";
import ArchiveCompartment from "ArchiveCompartment"
import KV from "embedded:storage/key-value";
import WebStorage from "webstorage";

const clearImmediate = Timer.clear;
const setImmediate = function(callback) { return Timer.set(callback) };
const setInterval = function(callback, delay) { return Timer.repeat(callback, delay) };
const setTimeout = function(callback, delay) { return Timer.set(callback, delay) };

const console = Object.freeze({
	log(...args) {
		trace(...args);
	}
});

class AppInfo {
	static get uuid() @ "xs_appinfo_get_uuid"
	static get name() @ "xs_appinfo_get_name"
	static get isWatchface() @ "xs_appinfo_get_isWatchface"
}

const state = {};		// for mutable runtime state

const blockedWatchFace = Object.freeze([
	"pebble/button"
]);

export default function() {
	const rocky = new Rocky({});

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
			rocky,
			screen,
			Date,
			Math
		};

		Object.defineProperty(globals, "localStorage", {
			enumerable: true,
			configurable: true,
			get() {
				state.localStorage ??= new WebStorage(KV.open({path: `local-${AppInfo.uuid}`}));
				return state.localStorage;
			}
		});

		const mod = new ArchiveCompartment(archive, {
			globals,
			modules: {},
			loadNowHook(specifier) {
				if (AppInfo.isWatchface && blockedWatchFace.includes(specifier))
					throw new Error(blockedWatchFace + " blocked in watchface");

				return {namespace: specifier};		// map through host modules
			}
		});
		mod.importNow("main");
	}
	catch (e) {
		console.log(`Error loading mod in app "${AppInfo.name}": ${e}`);
		console.log(e.stack);
	}
}
