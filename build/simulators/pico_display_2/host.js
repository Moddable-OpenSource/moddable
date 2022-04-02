import Button from "button";
import Control from "control";

class A {
	constructor(options) {
		return new Button({
			...options,
			buttonKey: 'aButton',
		});
	}
}
class B {
	constructor(options) {
		return new Button({
			...options,
			buttonKey: 'bButton',
		});
	}
}
class X {
	constructor(options) {
		return new Button({
			...options,
			buttonKey: 'xButton',
		});
	}
}
class Y {
	constructor(options) {
		return new Button({
			...options,
			buttonKey: 'yButton',
		});
	}
}

class RGBLED extends Control {
	close() {
		this.write(0);
	}
	write(led) {
		led ??= {red: 0, green: 0, blue: 0};
		this.postJSON({led});
	}
}

globalThis.Host = Object.freeze({
	Button: {
		Default: A,
		A,
		B,
		X,
		Y
	},
	LED: { Default: RGBLED },
}, true);
