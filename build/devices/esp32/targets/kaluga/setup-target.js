import config from "mc/config";
import NEOPIXEL from "neopixel";
import Timer from "timer";
import Analog from "pins/analog";

const BUTTON_TOLERANCE = 10;
const BUTTON_VALUES = Object.freeze([750 + BUTTON_TOLERANCE, 615 + BUTTON_TOLERANCE, 515 + BUTTON_TOLERANCE, 347 + BUTTON_TOLERANCE, 255 + BUTTON_TOLERANCE, 119 + BUTTON_TOLERANCE]);

class ButtonArray {
	#callback;
	#pushed;
	constructor(options){
		const delay = options.delay ?? 50;
		this.#callback = options.onPush;
		Timer.repeat( id => {
			const value = Analog.read(config.buttonArray);
			if (value > BUTTON_VALUES[0]) {
				if (this.#pushed === undefined)
					return;
				this.#callback(this.#pushed, 0);
				this.#pushed = undefined;
			}
			for (let i = 5; i >= 0; i--) {
				if (value < BUTTON_VALUES[i]) {
					if (i !== this.#pushed){
						if (this.#pushed !== undefined)
							this.#callback(this.#pushed, 0);
						this.#pushed = i;
						this.#callback(i, 1);
					}
					break;
				}
			}
		}, delay);
	}
}

globalThis.Host = Object.freeze({
	ButtonArray,
	NeoPixel: class {
		constructor(options) {
			return new NEOPIXEL({
				...options,
				length: 1, 
				pin: config.neopixel, 
				order: "GRB"
			});
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
	if (config.rainbow){
		const np = new Host.NeoPixel;
		globalThis.neopixel = np;
		const STEP = 3;
		
		let rgb = [0, 0, 0];
		let phase = 0;

		np.rainbowTimer = Timer.repeat(() => {
			let advance;
			for (let i = 0; i < 3; i++) {
				const direction = phases[i][phase];
				rgb[i] += direction * STEP;
				if (direction) {
					if (rgb[i] >= 255){
						rgb[i] = 255;
						advance = true;
					}
					else if (rgb[i] <= 0){
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
