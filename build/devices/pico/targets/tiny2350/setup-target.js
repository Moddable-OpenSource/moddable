import Digital from "pins/digital";
import config from "mc/config";
import LED from "led";
import Button from "button";

class A {
	constructor(options) {
		return new Button({...options, invert: true, pin: 23});
	}
}

globalThis.Host = Object.freeze({
	LED: {
		Default: class {
			#pin = {
				red: new Digital(18, Digital.Output),
				green: new Digital(19, Digital.Output),
				blue: new Digital(20, Digital.Output)
			}
			constructor() {
				this.write({red: 1, green: 1, blue: 1});
			}
			close() {
				this.write({red: 1, green: 1, blue: 1});
				this.#pin.red.close();
				this.#pin.green.close();
				this.#pin.blue.close();
			}
			write(color) {
				color ??= {red: 0, green: 0, blue: 0};
				this.#pin.red.write((color.red >= 128) ? 0 : 1);
				this.#pin.green.write((color.green >= 128) ? 0 : 1);
				this.#pin.blue.write((color.blue >= 128) ? 0 : 1);
			}
		}
	},
	Button: {
		Default: A,
		A
	}
}, true);

export default function (done) {

	done();
}
