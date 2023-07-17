import Control from "control";
export default class extends Control {
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
	}
	onJSON(json) {
		const jogdial = json.jogdial;
		if (jogdial) {
			if (jogdial.turn) {
				if (jogdial.push)
					this.#onPushAndTurn.call(this.#target, jogdial.turn);
				else
					this.#onTurn.call(this.#target, jogdial.turn);
			}
			else if (jogdial.push == 1) {
				this.#onPush.call(this.#target, 1);
			}
			else if (jogdial.push == 0) {
				this.#onPush.call(this.#target, 0);
			}
			
		}
	}
};
