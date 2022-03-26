import Control from "control";
export default class extends Control {
	#onPush;
	#pressed;
	#buttonKey;
	constructor(options) {
		super();
		this.#onPush = options.onPush ?? this.onPush;
		this.#buttonKey = options.buttonKey ?? "button";
		if (options.target)
			this.target = options.target;
	}
	onJSON(json) {
		const pressed = json[this.#buttonKey];
		if (undefined === pressed)
			return;
		this.#pressed = pressed; 
		this.#onPush();
	}
	read() {
		return this.#pressed;
	}
	get pressed() {
		return this.#pressed;
	}
};
