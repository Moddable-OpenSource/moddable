function onKeyDown(key, repeat) {
	if (this.focus)
		this.focus.onKeyDown(key.charCodeAt(0), repeat);
}
function onKeyUp(key) {
	if (this.focus)
		this.focus.onKeyUp(key.charCodeAt(0));
}
export default class {
	#former;
	constructor(options) {
		let context = screen.context;
		if (!context.focus) {
			context.onKeyDown = onKeyDown;
			context.onKeyUp = onKeyUp;
		}
		this.#former = context.focus;
		context.focus = this;
	}
	close() {
		let context = screen.context;
		let current = context.focus;
		let former = null;
		while (current) {
			if (current == this) {
				if (former)
					former.#former = current.#former;
				else
					context.focus = current.#former;
				break;
			}
			former = current;
			current = current.#former;
		}
	}
	onCodeDown(code) {
	}
	onCodeRepeat(code) {
	}
	onCodeUp(code) {
	}
	onKeyDown(code, repeat) {
		let current = this;
		if (repeat & 1) {
			while (current) {
				current.onCodeRepeat(code, repeat);
				current = current.#former;
			}
		}
		else {
			while (current) {
				current.onCodeDown(code, repeat);
				current = current.#former;
			}
		}
	}
	onKeyUp(code) {
		let current = this;
		while (current) {
			current.onCodeUp(code);
			current = current.#former;
		}
	}
};
