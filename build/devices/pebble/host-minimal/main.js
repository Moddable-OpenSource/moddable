import Timer from "timer"
import ArchiveResource from "pebble/archive-resource";
import ArchiveCompartment from "ArchiveCompartment"

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
			Math
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
