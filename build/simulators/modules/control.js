export default class Control {
	#former;
	static #onMessage(message) {
		let current = this.focus;
		if (current) {
			const json = JSON.parse(message);
			while (current) {
				current.onJSON(json);
				current = current.#former;
			}
		}
	}
	constructor(options) {
		if (!screen.focus) {
			screen.onMessage = Control.#onMessage;
		}
		this.#former = screen.focus;
		screen.focus = this;
	}
	close() {
		let current = screen.focus;
		let former = null;
		while (current) {
			if (current == this) {
				if (former)
					former.#former = current.#former;
				else
					screen.focus = current.#former;
				break;
			}
			former = current;
			current = current.#former;
		}
	}
	onJSON(json) {
	}
	postJSON(json) {
		screen.postMessage(JSON.stringify(json));
	}
};
