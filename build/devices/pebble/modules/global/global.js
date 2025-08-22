/*
	to do:

		once list for immediate callback – so can be removed in cancel called immediately
		#ticks relies on being called ontime... which isn't guaranteed
*/

import Timer from "timer";

export class Pebble {
	static events = Object.freeze([
		"secondchange",	
		"minutechange",
		"hourchange",
		"daychange"
	]);
	#events = new Map;
	#timeChange;

	addEventListener(event, callback) {
		if (!Pebble.events.includes(event))
			return;

		if (!this.#events.has(event))
			this.#events.set(event, [callback]);
		else
			this.#events.get(event).push(callback);

		// immediate for first one
		Timer.set(() => {
			callback({date: new Date});
		});

		this.#schedule();
	}
	#schedule() {
		const seconds = this.#events.has("secondchange"), minutes = this.#events.has("minutechange"),
						hours = this.#events.has("hourchange"), days = this.#events.has("daychange")  
		if (!seconds && !minutes && !hours && !days) {
			Timer.clear(this.#timeChange);
			this.#timeChange = undefined;
			return;
		}

		this.#timeChange ??= Timer.repeat(() => this.#tick(), 1000);
		let interval = 1000;
		if (seconds)
			;
		else if (minutes)
			interval *= 60;
		else if (hours)
			interval *= 60 * 60;
		else
			interval *= 60 * 60 * 24;

		Timer.schedule(this.#timeChange, interval - (Date.now() % interval) + 10, interval);		// +10 because sometimes the initial fires early otherwise
	}
	removeEventListener(event, callback) {
		const list = this.#events.get(event);
		if (!list)
			return;
		const index = list.inexOf(callback);
		if (index < 0) return;

		list.splice(index, 1);
		if (0 === list.length) {
			this.#events.delete(event);
			this.#schedule();
		}
	}
	#do(event, arg) {
		try {
			this.#events.get(event)?.forEach(cb => cb(arg));
		}
		catch (e) {
			trace(`Event ("${event}") exception: ${e.toString()}\n`);
			trace(e.stack, "\n");
		}
	}
	#tick() {
		const now = new Date;
		if (this.#events.has("secondchange"))
			this.#do("secondchange", {date: new Date(now)});
		if (this.#events.has("minutechange") && !now.getSeconds())
			this.#do("minutechange", {date: new Date(now)});
		if (this.#events.has("hourchange") && !now.getSeconds() && !now.getMinutes())
			this.#do("hourchange", {date: new Date(now)});
		if (this.#events.has("daychange") && !now.getSeconds() && !now.getMinutes() && !now.getHours())
			this.#do("daychange", {date: new Date(now)});
	}
}

export default Pebble;
