/*
	When Wi-Fi is enabled, the M5* buttons fire often. This debounces them.
*/

import Digital from "pins/digital";
import Monitor from "pins/digital/monitor";

class M5Button {
	#monitor;
	#debounce;

	constructor(pin) {
		this.#monitor = new Monitor({pin, mode: Digital.InputPullUp, edge: Monitor.Rising | Monitor.Falling});
		this.#debounce = this.#monitor.read();
		this.#monitor.onChanged = () => {
			const debounce = this.#monitor.read();
			if (debounce === this.#debounce)
				return;

			this.#debounce = debounce;
			this.onChanged?.();
		};
	}
	close() {
		this.#monitor.close();
		this.#monitor = undefind;
	}
	read() {
		return this.#monitor.read();
	}
}

export default M5Button;
