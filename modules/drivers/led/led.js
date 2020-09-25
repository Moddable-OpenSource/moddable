import Digital from "pins/digital";

class LED extends Digital {
	#on;

	constructor(options = {}) {
		super({
			pin: options.pin,
			mode: options.mode ?? Digital.Output,
		});

		this.#on = options.invert ? 0 : 1;
	}
	read() {
		const value = super.read();
		return this.#on ? value : 1 - value;
	}
	write(value) {
		super.write(this.#on ? value : 1 - value);
	}
	on() {
		super.write(this.#on);
	}
	off() {
		super.write(1 - this.#on);
	}
}

export default LED;
