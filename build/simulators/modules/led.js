import Control from "control";
export default class extends Control {
	#on;
	#variant;
	constructor(options = {}) {
		super();
		this.#variant = options.variant ?? 0;
		this.#on = options.invert ? 0 : 1;
	}
	read() {
		const value = screen.readLED();
		return this.#on ? value : 1 - value;
	}
	write(value) {
		this.postJSON({led:this.#on ? value : 1 - value, variant:this.#variant});
	}
	on() {
		this.write(this.#on);
	}
	off() {
		this.write(1 - this.#on);
	}
}
