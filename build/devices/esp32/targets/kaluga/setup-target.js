import Digital from "pins/digital";
import config from "mc/config";
import NEOPIXEL from "neopixel";
import Timer from "timer";

globalThis.Host = Object.freeze({
	NeoPixel: class {
		constructor(options = {}){
			return new NEOPIXEL({
				...options,
				length: 1, 
				pin: 45, 
				order: "GRB"
			});
		}
	}
}, true);

export default function (done) {
	if (config.rainbow){
		globalThis.neopixel = new NEOPIXEL({length: 1, pin: 45, order: "GRB"});
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