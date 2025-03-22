import Digital from "pins/digital";
import config from "mc/config";
import Button from "button";
import LED from "led";

class Flash {
	constructor(options) {
		return new Button({
			...options,
			pin: 45,
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
					pin: 25
				});
			}
		}
	},
	Button: {
		Default: Flash,
		Flash
	}
}, true);

