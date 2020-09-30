import Digital from "pins/digital";
import PWM from "pins/pwm";
import config from "mc/config";
import Button from "button";
import LED from "led";

class Backlight extends PWM {
	constructor() {
		super({pin: config.backlight});
		this.write(parseInt(config.brightness));
	}
	write(value) {
		value = 100 - parseInt(value);		// PWM is inverted
		if (value < 0)
			value = 0;
		else if (value > 100)
			value = 100;
		value = (value / 100) * 1023;
		super.write(value);
	}
}

globalThis.Host = {
	Backlight,
	LED: {
		Default: class {
			constructor(options) {
				return new LED({
					...options,
					pin: 2
				});
			}
		}
	},
	Button: {
		Default: class {
			constructor(options) {
				return new Button({
					...options,
					pin: 0,
					invert: true
				});
			}
		}
	}
}
Object.freeze(globalThis.Host, true);

export default function (done) {
	if ("none" === config.brightness)
		Digital.write(config.backlight, 0);

	done();
}
