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
		this.#pressed = json[this.#buttonKey]; 
		this.#onPush();
	}
	read() {
		return this.#pressed;
	}
	get pressed() {
		return this.#pressed;
	}
};
