import Digital from "pins/digital";
import config from "mc/config";
import LED from "led";

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
}, true);

export default function (done) {
	Digital.write(config.backlight, 1);

	done();
}
