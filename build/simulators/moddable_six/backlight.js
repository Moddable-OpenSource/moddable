import Control from "control";

export default class extends Control {
	#brightness;
	#fade = 0;
	constructor(it = 100) {
		super();
		this.write(it);
	}
	configure(options) {
		if ("fade" in options)
			this.#fade = options.fade;
	}
	read() {
		return this.#brightness;
	}
	write(it) {
		if (it < 0)
			it = 0;
		else if (it > 100)
			it = 100;
		if (this.#brightness != it) {
			this.#brightness = it;
			this.postJSON({ backlight:it, fade:this.#fade });
		}
	}
}
