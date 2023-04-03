import Digital from "pins/digital";
//import Monitor from "monitor";
import M5Button from "m5button";
import config from "mc/config";
import Timer from "timer";
import Button from "button";
import I2C from "pins/i2c";
import NeoPixel from "neopixel";

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
	Button: {
		Default: Flash,
		Flash
	}
}, true);

export default function (done) {
	globalThis.button = {
		a: new M5Button(41)
	};

	globalThis.lights = new NeoPixel({});

	done();
}

