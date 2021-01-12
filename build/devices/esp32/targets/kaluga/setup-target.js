import config from "mc/config";
import NeoPixel from "neopixel";
import Timer from "timer";
import Analog from "pins/analog";
import DigitalButton from "button";

const BUTTON_TOLERANCE = 10;
const BUTTON_VALUES = [750 + BUTTON_TOLERANCE, 615 + BUTTON_TOLERANCE, 515 + BUTTON_TOLERANCE, 347 + BUTTON_TOLERANCE, 255 + BUTTON_TOLERANCE, 119 + BUTTON_TOLERANCE];

class Button {
	static #state = {
		active: {},
		pushed: undefined,
		timer: undefined
	};
	
	#button;
	#onPush;

	constructor(options) {
		this.#button = options.button;
		this.#onPush = options.onPush;

		if (Button.#state.active[this.#button])
			throw new Error("in use");

		Button.#state.active[this.#button] = this;

		if (Button.#state.timer)
			return;

		Button.#state.timer = Timer.repeat( () => {
			const value = Analog.read(config.buttonArray.pin);
			if (value > BUTTON_VALUES[0]) {
				if (Button.#state.pushed === undefined)
					return;
				Button.#state.active[Button.#state.pushed]?.#onPush?.(0);
				Button.#state.pushed = undefined;
			}
			for (let i = 5; i >= 0; i--) {
				if (value < BUTTON_VALUES[i]) {
					if (i !== Button.#state.pushed) {
						if (Button.#state.pushed !== undefined)
							Button.#state.active[Button.#state.pushed]?.#onPush?.(0);
						Button.#state.pushed = i;
						Button.#state.active[i]?.#onPush?.(1);
					}
					break;
				}
			}
		}, config.buttonArray.delay ?? 50);
	}

	close() {
		if (undefined === this.#button)
			return;
		
		delete Button.#state.active[this.#button];
		this.#button = undefined;

		if (Object.keys(Button.#state.active).length)
			return;

		Timer.clear(Button.#state.timer);
		Button.#state.timer = undefined;
	}

	read() {
		return (Button.#state.pushed === this.#button) ? 1 : 0;
	}

	get pressed() {
		return (Button.#state.pushed === this.#button);
	}
}

function create(button) {
	const i = button;
	return class {
		constructor(options) {
			return new Button({
				...options,
				button: i
			});
		}
	};
}

class Flash {
	constructor(options) {
		return new DigitalButton({
			...options,
			pin: 0,
			invert: true
		});
	}
}

class NeoPixelLED extends NeoPixel {
	#value = 0;
	read() {
		return this.#value;
	}
	write(value) {
		this.#value = value;
		if (value) {
			super.setPixel(0, super.makeRGB(255, 255, 255));
			
		}else{
			super.setPixel(0, super.makeRGB(0, 0, 0));
		}
		super.update();
	}
	on() {
		this.write(1);
	}
	off() {
		this.write(0);
	}
}

globalThis.Host = {
	Button: {
		Default: Flash,
		Flash,
		A: create(0),
		B: create(1),
		C: create(2),
		D: create(3),
		E: create(4),
		F: create(5)
	},
	LED: {
		Default: class {
			constructor(options) {
				const led = new NeoPixelLED({
					...options,
					length: 1, 
					pin: config.led.pin, 
					order: "GRB"
				});
				led.brightness = config.led.brightness;
				return led;
			}
		} 
	}
};

const phases = [
	//red, purple, blue, cyan, green, orange, white, black
	[1, 0, -1, 0, 0, 1, 0, -1],
	[0, 0, 0, 1, 0, 0, 0, -1],
	[0, 1, 0, 0, -1, 0, 1, -1]
];
Object.freeze({phases, Host: globalThis.Host, BUTTON_VALUES}, true);

export default function (done) {
	if (config.led.rainbow) {
		const neopixel = new Host.LED.Default;
		const STEP = 3;

		let rgb = [0, 0, 0];
		let phase = 0;

		Timer.repeat(() => {
			let advance;
			for (let i = 0; i < 3; i++) {
				const direction = phases[i][phase];
				if (!direction)
					continue;

				rgb[i] += direction * STEP;
				if (rgb[i] >= 255) {
					rgb[i] = 255;
					advance = true;
				}
				else if (rgb[i] <= 0) {
					rgb[i] = 0;
					advance = true;
				}
			}
			if (advance && (++phase >= phases[0].length))
				phase = 0;
	
			neopixel.setPixel(0, neopixel.makeRGB(rgb[0], rgb[1], rgb[2]));
			neopixel.update();
		}, 33);
	}

	done();
}
