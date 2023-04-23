import config from "mc/config";
import Button from "button";

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

export default function (done) {
	done();
}
