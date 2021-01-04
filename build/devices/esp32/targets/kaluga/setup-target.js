import config from "mc/config";
import NEOPIXEL from "neopixel";
import Timer from "timer";
import Analog from "pins/analog";

const BUTTON_TOLERANCE = 10;
const BUTTON_VALUES = [750 + BUTTON_TOLERANCE, 615 + BUTTON_TOLERANCE, 515 + BUTTON_TOLERANCE, 347 + BUTTON_TOLERANCE, 255 + BUTTON_TOLERANCE, 119 + BUTTON_TOLERANCE];

class ButtonArray {
	#callback;
	#pushed;
	constructor(options){
		let delay = 50;
		this.#callback = options.onPush;
		if (options.delay !== undefined) delay = options.delay;
		Timer.repeat( id => {
			const value = Analog.read(config.buttonArray);
			if (value > BUTTON_VALUES[0]){
				if (this.#pushed === undefined) return;
				this.#callback.call(this, this.#pushed, 0);
				this.#pushed = undefined;
			}
			for (let i = 5; i >= 0; i--){
				if (value < BUTTON_VALUES[i]){
					if (i !== this.#pushed){
						if (this.#pushed !== undefined) this.#callback.call(this, this.#pushed, 0);
						this.#pushed = i;
						this.#callback.call(this, i, 1);
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
		constructor(options = {}){
			return new NEOPIXEL({
				...options,
				length: 1, 
				pin: config.neopixel, 
				order: "GRB"
			});
		}
	}
}, true);

export default function (done) {
	if (config.rainbow){
		globalThis.neopixel = new NEOPIXEL({length: 1, pin: config.neopixel, order: "GRB"});
		const np = globalThis.neopixel;
		const STEP = 3;
		
		let rgb = [0, 0, 0];
		let phase = 0;
		let phases = [
			//red, purple, blue, cyan, green, orange, white, black
			[1, 0, -1, 0, 0, 1, 0, -1],
			[0, 0, 0, 1, 0, 0, 0, -1],
			[0, 1, 0, 0, -1, 0, 1, -1]
		]

		np.rainbowTimer = Timer.repeat(() => {
			let advance = false;
			for (let i = 0; i < 3; i++){
				const direction = phases[i][phase];
				rgb[i] += direction * STEP;
				if (direction){
					if (rgb[i] >= 255){
						rgb[i] = 255;
						advance = true;
					}
					if (rgb[i] <= 0){
						rgb[i] = 0;
						advance = true;
					}
				}
			}
			if (advance)
				if (++phase >= phases[0].length) phase = 0;
	
			np.setPixel(0, np.makeRGB(rgb[0], rgb[1], rgb[2]));
			np.update();
		}, 33);
	}


	done();
}