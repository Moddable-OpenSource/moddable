class LED {
	#on;
	constructor(options = {}) {
		this.#on = options.invert ? 0 : 1;
	}
	read() {
		const value = screen.readLED();
		return this.#on ? value : 1 - value;
	}
	write(value) {
		screen.writeLED(this.#on ? value : 1 - value);
	}
	on() {
		this.write(this.#on);
	}
	off() {
		this.write(1 - this.#on);
	}
}

export default LED;
