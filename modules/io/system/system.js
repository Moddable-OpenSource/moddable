import Timer from "timer";

class System {
	static deepSleep() @ "xs_system_deepSleep"
	static restart() @ "xs_system_restart"

	static resolve(name, callback) @ "xs_system_resolve"

	static setTimeout(callback, delay) {
		return Timer.set(callback, delay);
	}
	static clearTimeout(id) {
		Timer.clear(id);
	}
	static setInterval(callback, delay) {
		return Timer.repeat(callback, delay);
	}
	static clearInterval(id) {
		Timer.clear(id);
	}
}

global.System = System;
