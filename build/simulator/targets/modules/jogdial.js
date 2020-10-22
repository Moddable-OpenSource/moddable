import Sensor from "sensor";
export default class extends Sensor {
	#onPush;
	#onTurn;
	#onPushAndTurn;
	#target;
	#value;
	constructor(options) {
		super(options);
		this.#onPush = options.onPush ?? this.onPush;
		this.#onTurn = options.onTurn ?? this.onTurn;
		this.#onPushAndTurn = options.onPushAndTurn ?? this.onPushAndTurn ?? this.#onTurn;
		this.#target = options.target ?? this;
		this.#value = 0;
	}
	onCodeDown(code) {
		if (code == 13) {
			this.#value = 1;
			this.#onPush.call(this.#target, 1);
		}
		else if (code == 63232) {
			if (this.#value)
				this.#onPushAndTurn.call(this.#target, -4);
			else
				this.#onTurn.call(this.#target, -4);
		}
		else if (code == 63233) {
			if (this.#value)
				this.#onPushAndTurn.call(this.#target, 4);
			else
				this.#onTurn.call(this.#target, 4);
		}
	}
	onCodeRepeat(code) {
		if (code == 63232) {
			if (this.#value)
				this.#onPushAndTurn.call(this.#target, -1);
			else
				this.#onTurn.call(this.#target, -1);
		}
		else if (code == 63233) {
			if (this.#value)
				this.#onPushAndTurn.call(this.#target, 1);
			else
				this.#onTurn.call(this.#target, 1);
		}
	}
	onCodeUp(code) {
		if (code == 13) {
			this.#value = 0;
			this.#onPush.call(this.#target, 0);
		}
	}
};
