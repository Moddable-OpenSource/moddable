import Control from "control";

export default class extends Control {
	#brightness;
	constructor(it = 100) {
		super();
		this.write(it);
	}
	read() {
		return this.#brightness;
	}
	write(it) {
		if (it < 0)
			it = 0;
		else if (it > 100)
			value = 100;
		if (this.#brightness != it) {
			this.#brightness = it;
			this.postJSON({ backlight:it });
		}
	}
}
