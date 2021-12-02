import Control from "control";
export default class extends Control {
	#onPush;
	#pressed
	constructor(options) {
		super();
		this.#onPush = options.onPush ?? this.onPush;
		if (options.target)
			this.target = options.target;
	}
	onJSON(json) {
		this.#pressed = json.button; 
		this.#onPush();
	}
	read() {
		return this.#pressed;
	}
	get pressed() {
		return this.#pressed;
	}
};
