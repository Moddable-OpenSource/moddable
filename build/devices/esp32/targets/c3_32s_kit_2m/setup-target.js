import config from "mc/config";
import Timer from "timer";
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
				return new LED({
                    ...options,
                    pin: config.led.pin
                });
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
	done?.();
}
