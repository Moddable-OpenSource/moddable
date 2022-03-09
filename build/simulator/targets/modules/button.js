import Sensor from "sensor";
export default class extends Sensor {
	#onPush;
	#target;
	constructor(options) {
		super(options);
		this.#onPush = options.onPush ?? this.onPush;
		this.#target = options.target ?? this;
	}
	onCodeUp(code) {
		if (code == 127)
			this.#onPush.call(this.#target, 1);
	}
};
