import Digital from "pins/digital";
import config from "mc/config";
import LED from "led";
import Button from "button";

class A {
	constructor(options) {
		return new Button({...options, invert: true, pin: 12});
	}
}

globalThis.Host = Object.freeze({
	Button: {
		Default: A,
		A,
		B: class {
			constructor(options) {
				return new Button({...options, invert: true, pin: 13});
			}
		},
		X: class {
			constructor(options) {
				return new Button({...options, invert: true, pin: 14});
			}
		},
		Y: class {
			constructor(options) {
				return new Button({...options, invert: true, pin: 15});
			}
		},
	}
}, true);

export default function (done) {
	done();
}
