import PulseCount from "pins/pulsecount";
import Digital from "pins/digital";
import Monitor from "pins/digital/monitor";

class JogDial {
	#target;
	#pulse;
	#button;
	#onPush;
	#onTurn;
	#onPushAndTurn;
	constructor(options) {
		this.#target = options.target ?? this;
		this.#onPush = options.onPush ?? this.onPush;
		this.#onTurn = options.onTurn ?? this.onTurn;
		this.#onPushAndTurn = options.onPushAndTurn ?? this.onPushAndTurn ?? this.#onTurn;

		this.#pulse = new PulseCount({
			signal: options.jogdial.signal,
			control: options.jogdial.control,
			filter: 1000,
			target: this,
			onReadable() {
				const value = -this.#pulse.read();
				const delta = value - this.#pulse.previous;
				this.#pulse.previous = value;

				if (this.#button.previous)
					this.#onTurn.call(this.#target, delta);
				else
					this.#onPushAndTurn.call(this.#target, delta);
			}
		});
		this.#pulse.previous = -this.#pulse.read();

		this.#button = new Monitor({
			pin: options.jogdial.button,
			mode: Digital.InputPullUp,
			edge: Monitor.Falling | Monitor.Rising
	   });
		this.#button.onChanged = this.#buttonChanged.bind(this);
		this.#button.previous = this.#button.read();
	}
	close() {
		this.#button?.close();
		this.#pulse?.close();
		this.#button =
		this.#pulse = null;
	}
	#buttonChanged() {
		const value = this.#button.read();
		if (value === this.#button.previous)
			return;

		this.#button.previous = value;
		this.#onPush.call(this.#target, value);
	}
}

export default JogDial;
