import Control from "control";
export default class extends Control {
	#retainedValues = [];
	#wakenWith = "";
	constructor(options = {}) {
		super();
	}
	getRetainedValue(index) {
		return this.#retainedValues[index];
	}
	onJSON(json) {
		const retain = json.retain;
		if (retain) {
			this.#retainedValues = retain;
			this.#wakenWith = json.wakenWith;
		}
	}
	setRetainedValue(index, value) {
		this.#retainedValues[index] = value;
	}
	sleep(options) {
		this.postJSON({ sleep:options.duration || 0, retain:this.#retainedValues });
	}
	get wakenWith() {
		return this.#wakenWith;
	}
	static setup() {
	}
}
