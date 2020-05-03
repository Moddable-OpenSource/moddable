/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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

import Digital from "pins/digital";
import Monitor from "pins/digital/monitor";

class PulseCount @ "xs_pulsecount_destructor" {
	#onChanged;
	#monitor;
	#pin;
	constructor(dictionary) {
		build.call(this, dictionary);
		this.#pin = dictionary.signal;
	}
	close() {
		terminate.call(this);
		if (this.#monitor)
			this.#monitor.close();
		this.#monitor = undefined;
	}
	get() @ "xs_pulsecount_get";
	set() @ "xs_pulsecount_set";
	set onChanged(value) {
		this.#onChanged = value;
		if (this.#onChanged) {
			if (this.#monitor)
				return;
			this.#monitor = new Monitor({
				pin: this.#pin,
				mode: Digital.InputPullUp,
				edge: Monitor.Falling | Monitor.Rising,
			});
			this.#monitor.previous = this.get();
			this.#monitor.target = this;
			this.#monitor.onChanged = () => {
				const value = this.get();
				if (value === this.#monitor.previous)
					return;

				this.#monitor.previous = value;
				this.#onChanged();
			}
		}
		else if (this.#monitor) {
			this.#monitor.close();
			this.#monitor = undefined;
		}
	}
	get onChanged() {
		return this.#onChanged;
	}
};
Object.freeze(PulseCount.prototype);

function build(dictionary) @ "xs_pulsecount"
function terminate() @ "xs_pulsecount_close";

export default PulseCount;
