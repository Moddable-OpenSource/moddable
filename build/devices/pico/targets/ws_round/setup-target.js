import Digital from "pins/digital";
import config from "mc/config";
import Timer from "timer";
import PWM from "pins/pwm";

class Backlight extends PWM {
	constructor(brightness = 100) {
		super({pin: config.backlight});
		this.write(brightness);
	}
	write(value) {
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
	Backlight
}, true);

export default function (done) {
	if ((undefined == config.brightness) || ("none" === config.brightness))
		Digital.write(config.backlight, 0);
	else if ("off" === config.backlight)
		Digital.write(config.backlight, 1);
	else {
		globalThis.backlight = new device.peripheral.Backlight;
		backlight.brightness = parseInt(config.brigtness) / 100;
	}

	Digital.write(config.lcd_rst_pin, 1);
	Timer.delay(100);
	Digital.write(config.lcd_rst_pin, 0);
	Timer.delay(100);
	Digital.write(config.lcd_rst_pin, 0);
	Digital.write(config.lcd_cs_pin, 0);
	Timer.delay(100);

	done();
}
