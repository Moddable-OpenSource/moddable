import Digital from "pins/digital";
import Monitor from "pins/digital/monitor";

class Button {
	#input;
	#pressed;

	constructor(options = {}) {
		if (options.target)
			this.target = options.target;

		this.#pressed = options.invert ? 0 : 1;
		if (options.onPush) {
			this.#input = new Monitor({
				mode: options.mode ?? Digital.InputPullUp,
				pin: options.pin,
				edge: Monitor.Rising | Monitor.Falling,
			});
			this.#input.onPush = options.onPush;
			this.#input.onChanged = () => {
				this.#input.onPush.call(this, this.read());
			};
		}
		else {
			this.#input = new Digital({
				pin: options.pin,
				mode: options.mode ?? Digital.InputPullUp,
			});
		}
	}
	close() {
		this.#input?.close();
		this.#input = undefined;
	}
	read() {
		return (this.#input.read() === this.#pressed) ? 1 : 0;
	}
	get pressed() {
		return this.read() ? true : false;
	}
}

export default Button;
