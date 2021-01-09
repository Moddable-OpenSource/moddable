import config from "mc/config";
import NeoPixel from "neopixel";
import Timer from "timer";
import Analog from "pins/analog";

const BUTTON_TOLERANCE = 10;
const BUTTON_VALUES = Object.freeze([750 + BUTTON_TOLERANCE, 615 + BUTTON_TOLERANCE, 515 + BUTTON_TOLERANCE, 347 + BUTTON_TOLERANCE, 255 + BUTTON_TOLERANCE, 119 + BUTTON_TOLERANCE]);

class ButtonArray {
	#buttons = {};
	#delay;
	#pushed;
	#timerID;

	constructor() {
		this.#delay = config.buttonArrayDelay ?? 50;
	}

	register(button, buttonNumber) {
		if (this.#buttons[buttonNumber])
			throw new Error(`Button ${buttonNumber} has already been created.`);

		if (this.#timerID === undefined)
			this.#startTimer();

		this.#buttons[buttonNumber] = button;
	}

	unregister(buttonNumber) {
		delete this.#buttons[buttonNumber];
		if (Object.keys(this.#buttons).length == 0)
			this.#stopTimer();
	}

	#startTimer() {
		this.#timerID = Timer.repeat( () => {
			const value = Analog.read(config.buttonArray);
			if (value > BUTTON_VALUES[0]) {
				if (this.#pushed === undefined)
					return;
				if (this.#buttons[this.#pushed])
					this.#buttons[this.#pushed].value = 0;
				this.#pushed = undefined;
			}
			for (let i = 5; i >= 0; i--) {
				if (value < BUTTON_VALUES[i]) {
					if (i !== this.#pushed) {
						if (this.#pushed !== undefined)
							if (this.#buttons[this.#pushed])
								this.#buttons[this.#pushed].value = 0;
						this.#pushed = i;
						if (this.#buttons[i])
							this.#buttons[i].value = 1;
					}
					break;
				}
			}
		}, this.#delay);
	}

	#stopTimer() {
		Timer.clear(this.#timerID);
		this.#timerID = undefined;
	}
}

let buttonArray;
class KalugaButton {
	#buttonNumber;
	#onPush;
	#value = 0;
	
	constructor(options) {
		this.#buttonNumber = options.buttonNumber;
		this.#onPush = options.onPush;

		if (undefined === buttonArray)
			buttonArray = new ButtonArray();

		buttonArray.register(this, options.buttonNumber);
	}

	set value(value) {
		this.#value = value;
		if (this.#onPush)
			this.#onPush(value);
	}

	close() {
		buttonArray.unregister(this.#buttonNumber);
	}

	read() {
		return this.#value;
	}

	get pressed() {
		return this.#value ? true : false;
	}
}

class A {
	constructor(options) {
		return new KalugaButton({
			...options,
			buttonNumber: 0
		});
	}
};

class B {
	constructor(options) {
		return new KalugaButton({
			...options,
			buttonNumber: 1
		});
	}
};

class C {
	constructor(options) {
		return new KalugaButton({
			...options,
			buttonNumber: 2
		});
	}
};

class D {
	constructor(options) {
		return new KalugaButton({
			...options,
			buttonNumber: 3
		});
	}
};

class E {
	constructor(options) {
		return new KalugaButton({
			...options,
			buttonNumber: 4
		});
	}
};

class F {
	constructor(options) {
		return new KalugaButton({
			...options,
			buttonNumber: 5
		});
	}
};

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

globalThis.Host = Object.freeze({
	Button: {
		Default: A,
		A,
		B,
		C,
		D,
		E,
		F
	},
	LED: {
		Default: class {
			constructor(options) {
				return new NeoPixelLED({
					...options,
					length: 1, 
					pin: config.neopixel, 
					order: "GRB"
				});
			}
		} 
	}
}, true);

const phases = Object.freeze([
	//red, purple, blue, cyan, green, orange, white, black
	[1, 0, -1, 0, 0, 1, 0, -1],
	[0, 0, 0, 1, 0, 0, 0, -1],
	[0, 1, 0, 0, -1, 0, 1, -1]
], true);

export default function (done) {
	if (config.rainbow) {
		const neopixel = new Host.LED.Default;
		const STEP = 3;
		
		let rgb = [0, 0, 0];
		let phase = 0;

		Timer.repeat(() => {
			let advance;
			for (let i = 0; i < 3; i++) {
				const direction = phases[i][phase];
				rgb[i] += direction * STEP;
				if (direction) {
					if (rgb[i] >= 255) {
						rgb[i] = 255;
						advance = true;
					}
					else if (rgb[i] <= 0) {
						rgb[i] = 0;
						advance = true;
					}
				}
			}
			if (advance)
				if (++phase >= phases[0].length) phase = 0;
	
			neopixel.setPixel(0, neopixel.makeRGB(rgb[0], rgb[1], rgb[2]));
			neopixel.update();
		}, 33);
	}

	done();
}
