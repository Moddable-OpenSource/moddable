import Timer from "timer";
import config from "mc/config";

const phases = Object.freeze([
	//red, purple, blue, cyan, green, orange, white, black
	[1, 0, -1, 0, 0, 1, 0, -1],
	[0, 0, 0, 1, 0, 0, 0, -1],
	[0, 1, 0, 0, -1, 0, 1, -1]
], true);

export default function (done) {
	
	const brightness = config.brightness;
	if (brightness !== 'none') {
		globalThis.backlight = new Host.Backlight;
		if (brightness !== undefined) {
			backlight.write(parseInt(brightness));
		}
	}

	if (config.led.rainbow) {
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
	done?.();
}

