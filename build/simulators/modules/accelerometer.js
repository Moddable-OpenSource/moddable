import Control from "control";
export default class extends Control {
	#value;
	constructor(options) {
		super(options);
		this.#value = { x:0, y:0, x:0 };
	}
	onJSON(json) {
		const accelerometer = json.accelerometer;
		if (accelerometer) {
			this.#value = accelerometer;
		}
	}
	sample() {
		const { x, y, z } = this.#value;
		return { x, y, z };
	}
};
