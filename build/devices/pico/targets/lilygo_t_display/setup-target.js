import Digital from "pins/digital";
import config from "mc/config";
import LED from "led";
import Button from "button";
import Timer from "timer";

class A {
	constructor(options) {
		return new Button({...options, invert: true, pin: 6});
	}
}

globalThis.Host = Object.freeze({
	LED: {
		Default: class {
			#pin = {
				red: new Digital(25, Digital.Output)
			}
			constructor() {
				this.write({red: 1});
			}
			close() {
				this.write({red: 1});
				this.#pin.red.close();
			}
			write(color) {
				color ??= {red: 0};
				this.#pin.red.write((color.red >= 128) ? 0 : 1);
			}
		}
	},
	Button: {
		Default: A,
		A,
		B: class {
			constructor(options) {
				return new Button({...options, invert: true, pin: 7});
			}
		}
	}
}, true);

export default function (done) {
	trace(`digital write (${config.display_power_pin}, 1)\n`);
	Digital.write(config.display_power_pin, 1);		//  PWR_ON
	Timer.delay(250);
	if (undefined !== config.backlight)
		Digital.write(config.backlight, 1);

	done();
}
