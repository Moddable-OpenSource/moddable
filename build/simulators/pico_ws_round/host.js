import Button from "button";
import LED from "led";

class A extends LED {
	constructor(options) {
		super({...options, variant:0 });
	}
};

class B extends LED {
	constructor(options) {
		super({...options, variant:1 });
	}
};

globalThis.Host = Object.freeze({
	Button: { Default: Button },
	LED: {
		Default: A,
		A,
		B,
	},
}, true);