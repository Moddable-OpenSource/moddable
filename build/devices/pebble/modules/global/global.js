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

/*
	to do:

		one list for immediate callback – so can be removed in cancel called immediately

		add event for screen updates
*/

import Timer from "timer";

const events = Object.freeze([
		"secondchange",	
		"minutechange",
		"hourchange",
		"daychange",
		// add non-time events after here
		"connected"
]);

const offset = 50;		// Pebble Timer callbacks can be early... that's really bad for a watchface. So, we schedule them late by this number of milliseconds to ensure they fall in the next interval (usually second)
function connected() { return native("xs_global_connected").call(this); };

export class Pebble {
	#events = new Map;
	#timeChange;

	addEventListener(event, callback) {
		const index = events.indexOf(event)
		if (index < 0)
			return;

		if (!this.#events.has(event))
			this.#events.set(event, [callback]);
		else
			this.#events.get(event).push(callback);

		if (index <= 3) {
			// immediate for first one
			Timer.set(() => {
				callback({date: new Date});
			});

			this.#schedule();
			return;
		}

		switch (event) {
			case "connected":
				if (1 === this.#events.get(event).length)
					connected(true);
				break;
		}
	}
	#schedule() {
		const seconds = this.#events.has("secondchange"), minutes = this.#events.has("minutechange"),
						hours = this.#events.has("hourchange"), days = this.#events.has("daychange");
		if (!seconds && !minutes && !hours && !days) {
			Timer.clear(this.#timeChange);
			this.#timeChange = undefined;
			return;
		}

		this.#timeChange ??= Timer.repeat(id => this.#tick(id), 1000);
		let interval = 1000;
		if (seconds)
			;
		else if (minutes)
			interval *= 60;
		else if (hours)
			interval *= 60 * 60;
		else
			interval *= 60 * 60 * 24;

		const now = new Date;
		Timer.schedule(this.#timeChange, offset + interval - (now.valueOf() % interval), interval);
		this.#timeChange.minutes ??= now.getMinutes();
		this.#timeChange.interval = interval;
	}
	removeEventListener(event, callback) {
		const list = this.#events.get(event);
		if (!list) return;

		const index = list.indexOf(callback);
		if (index < 0) return;

		list.splice(index, 1);
		if (0 !== list.length)
			return;
			
		this.#events.delete(event);
		switch (event) {
			case "connected":
				connected(false);
				break;

			default:
				this.#schedule();		// only needed for time events
				break;
		}
	}
	do(event, arg) {
		try {
			this.#events.get(event)?.forEach(cb => cb(arg));
		}
		catch (e) {
			trace(`Event ("${event}") exception: ${e.toString()}\n`);
			trace(e.stack, "\n");
		}
	}
	#tick(id) {
		const now = new Date();
		if (this.#events.has("secondchange"))
			this.do("secondchange", {date: new Date(now)});
		const minutes = now.getMinutes();
		if (minutes !== id.minutes) {
			id.minutes = minutes;

			if (this.#events.has("minutechange"))
				this.do("minutechange", {date: new Date(now)});

			if (!minutes) {
				if (this.#events.has("hourchange"))
					this.do("hourchange", {date: new Date(now)});
				if (this.#events.has("daychange") && !now.getHours())
					this.do("daychange", {date: new Date(now)});
			}
		}

		const interval = id.interval;
		Timer.schedule(id, offset + interval - (Date.now() % interval), interval);
	}

	get connected() {
		return connected();
	}

	get color() {return native("xs_global_color_get").call(this);}
	get round() {return native("xs_global_round_get").call(this);}
	get serialNumber() {return native("xs_global_serialNumber_get").call(this);}
	get language() {return native("xs_global_language_get").call(this);}
}

export default Pebble;
