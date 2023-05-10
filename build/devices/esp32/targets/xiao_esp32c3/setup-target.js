import config from "mc/config";
import Timer from "timer";
import Button from "button";
import Digital from "pins/digital";

class Flash {
	constructor(options) {
		return new Button({
			...options,
			pin: 9,
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
	done?.();
}
