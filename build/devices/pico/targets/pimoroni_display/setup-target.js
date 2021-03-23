import Digital from "pins/digital";
import config from "mc/config";
import LED from "led";
import Button from "button";

/*
class Backlight extends PWM {
	constructor(brightness = 100) {
		super({pin: config.backlight});
		this.write(brightness);
	}
	write(value) {
		value = 100 - value;		// PWM is inverted
		if (value <= 0)
			value = 0;
		else if (value >= 100)
			value = 1023;
		else
			value = (value / 100) * 1023;
		super.write(value);
	}
}
*/

class A {
	constructor(options) {
		return new Button({...options, invert: true, pin: 12});
	}
}

globalThis.Host = Object.freeze({
	LED: {
		Default: class {
			constructor(options) {
				return new LED({
					...options,
					pin: 25
				});
			}
		}
	},
	Button: {
		Default: A,
		A,
		B: class {
			constructor(options) {
				return new Button({...options, invert: true, pin: 13});
			}
		},
		X: class {
			constructor(options) {
				return new Button({...options, invert: true, pin: 14});
			}
		},
		Y: class {
			constructor(options) {
				return new Button({...options, invert: true, pin: 15});
			}
		},
	}
}, true);

export default function (done) {
	if (undefined !== config.backlight)
		Digital.write(config.backlight, 1);
	if (undefined !== config.rgb_r)
		Digital.write(config.rgb_r, 1);
	if (undefined !== config.rgb_g)
		Digital.write(config.rgb_g, 1);
	if (undefined !== config.rgb_b)
		Digital.write(config.rgb_b, 1);

	done();
}
