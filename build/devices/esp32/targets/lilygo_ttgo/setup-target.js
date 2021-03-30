import Digital from "pins/digital";
import Monitor from "monitor";
import Timer from "timer";

import config from "mc/config";

export default function (done) {
	Digital.write(config.backlight, 0);

	global.button = {
		a: new Monitor({
			pin: 0,
			mode: Digital.InputPullUp,
			edge: Monitor.Rising | Monitor.Falling
		}),
		b: new Monitor({
			pin: 35,
			mode: Digital.InputPullUp,
			edge: Monitor.Rising | Monitor.Falling
		}),
	};
	button.a.onChanged = button.b.onChanged = nop;

	done();
}

function nop() {}

