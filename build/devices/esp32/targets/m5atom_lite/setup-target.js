import Digital from "pins/digital";
import Monitor from "pins/digital/monitor";

import NeoPixel from "neopixel";

export default function (done) {
	globalThis.button = {
		a: new Monitor({pin: 39, mode: Digital.InputPullUp, edge: Monitor.Rising | Monitor.Falling}),
	};
	button.a.onChanged = nop;
	
	globalThis.lights = new NeoPixel({});

	done();
}

function nop() {
}
