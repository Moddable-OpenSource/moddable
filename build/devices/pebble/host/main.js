import Timer from "timer"
import Resource from "Resource"
import {Rocky} from "pebble/graphics"
import ArchiveResource from "pebble/archive-resource";
import ArchiveCompartment from "ArchiveCompartment"
import KV from "embedded:storage/key-value";
import Poco from "commodetto/Poco";
import WebStorage from "webstorage";
import {} from "piu/MC"

const clearImmediate = Timer.clear;
const setImmediate = function(callback) { return Timer.set(callback) };
const setInterval = function(callback, delay) { return Timer.repeat(callback, delay) };
const setTimeout = function(callback, delay) { return Timer.set(callback, delay) };

const console = Object.freeze({
	log: trace
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

class TextureArchive extends Texture {
	constructor (options, alphaBitmap, colorBitmap) {
		if (alphaBitmap || colorBitmap)
			super(options, alphaBitmap, colorBitmap);
		else {
			const t = typeof options;
			if ("number" === t)
				super(undefined, undefined, new Poco.PebbleBitmap(options));
			else if ("object" === t)
				super({...options, archive: state.archive});
			else
				super({path: options, archive: state.archive});
		}
	}
}

class ResourceArchive extends Resource {
	constructor(path) {
		super(path, state.archive);
	}
}

class StyleArchive extends Style {
	constructor(options) {
		super({...options, archive: state.archive});
	}
}

export default function() {
	const rocky = new Rocky({});

	try {
		const r = new ArchiveResource(0);		// mod is in resource 0 in example. make this configurable.
		state.archive = r.archive;
		console.log(`Found mod "${state.archive.name}"`);

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
			Math,
			Resource: ResourceArchive,

			// Piu
			Application,
			Behavior,
			Container,
			Content,
			Label,
			Link,
			Skin,
			Style: StyleArchive,
			Text,
			Texture: TextureArchive
		};

		Object.defineProperty(globals, "localStorage", {
			enumerable: true,
			configurable: true,
			get() {
				state.localStorage ??= new WebStorage(KV.open({path: `local-${AppInfo.uuid}`}));
				return state.localStorage;
			}
		});

		const mod = new ArchiveCompartment(state.archive, {
			globals,
			modules: {},
			loadNowHook(specifier) {
				if (AppInfo.isWatchface && blockedWatchFace.includes(specifier))
					throw new Error(specifier + " blocked in watchface");

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
