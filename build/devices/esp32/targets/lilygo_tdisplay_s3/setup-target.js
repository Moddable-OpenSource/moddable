import config from "mc/config";
import NeoPixel from "neopixel";
import Timer from "timer";
import Button from "button";
import Digital from "pins/digital";
import PWM from "pins/pwm";

// lilygo t-display esp32-s3 has ESP32-S3R8 (8 MB PSRAM, 16 MB Flash)
class Backlight extends PWM {
	constructor(brightness = 100) {
		super({pin: config.backlight});
		this.write(brightness);
	}
	write(value) {
//		value = 100 - value;        // PWM is inverted
		if (value <= 0)
			value = 0;
		else if (value >= 100)
			value = 1023;
		else
			value = (value / 100) * 1023;
		super.write(value);
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
	Button: {
		Default: Flash,
		Flash
	}
}, true);

export default function (done) {

//	Digital.write(config.backlight, 1);		// ON (but 1 is OFF on Moddable Two)
    if ((undefined === config.brightness) || ("none" === config.brightness))
        Digital.write(config.backlight, 0);
    else if ("off" === config.brightness)
        Digital.write(config.backlight, 1);
    else
        globalThis.backlight = new Backlight(parseInt(config.brightness));

	if (config.lcd_pin)
		Digital.write(config.lcd_pin, 1);

	done?.();
}
