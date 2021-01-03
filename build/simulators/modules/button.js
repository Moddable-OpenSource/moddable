import Control from "control";
export default class extends Control {
	#onPush;
	#target;
	constructor(options) {
		super();
		this.#onPush = options.onPush ?? this.onPush;
		this.#target = options.target ?? this;
	}
	onJSON(json) {
		if (json.button == 1)
			this.#onPush.call(this.#target, 1);
	}
};
