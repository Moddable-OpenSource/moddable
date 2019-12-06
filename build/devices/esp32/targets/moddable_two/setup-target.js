import Digital from "pins/digital";
import PWM from "pins/pwm";
import config from "mc/config";

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
		super.write((value / 100) * 1023);
	}
}

export default function (done) {
	if ("none" !== config.brightness)
		globalThis.backlight = new Backlight;
	else
		Digital.write(config.backlight, 0);

	done();
}
