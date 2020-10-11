import Digital from "pins/digital";
import PWM from "pins/pwm";
import config from "mc/config";
import Button from "button";
import LED from "led";

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

globalThis.Host = Object.freeze({
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
}, true);

export default function (done) {
	if ((undefined === config.brightness) || ("none" === config.brightness))
		Digital.write(config.backlight, 0);
	else
		globalThis.backlight = new Backlight(parseInt(config.brightness));

	done();
}
