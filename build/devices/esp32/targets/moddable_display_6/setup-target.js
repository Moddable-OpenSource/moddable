import config from "mc/config";
import NeoPixel from "neopixel";
import Timer from "timer";
import Button from "button";
import PWM from "pins/pwm";

class Backlight extends PWM {
	constructor(brightness = 100) {
		super({pin: config.backlight});
		this.write(brightness);
	}
	write(value) {
		value /= 100;
		if (value <= 0)
			value = 1023;
		else if (value >= 1)
			value = 0;
		else {
			value *= value;		// linear
			value = 1 - value;	// PWM is inverted from brightness (0 is full power, 1023 is no power)
			value *= 1023;
		}
		super.write(value);
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

class Flash {
	constructor(options) {
		return new Button({
			...options,
			pin: 0,
			invert: true
		});
	}
}

globalThis.Host = Object.freeze({
	Backlight,
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
    },
	Button: {
		Default: Flash,
		Flash
	}
}, true);

const phases = Object.freeze([
	//red, purple, blue, cyan, green, orange, white, black
	[1, 0, -1, 0, 0, 1, 0, -1],
	[0, 0, 0, 1, 0, 0, 0, -1],
	[0, 1, 0, 0, -1, 0, 1, -1]
], true);

export default function (done) {
	if (config.led.rainbow){
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
