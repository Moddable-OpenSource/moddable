import Digital from "pins/digital";
import Monitor from "monitor";

import config from "mc/config";

import NeoPixel from "neopixel";

export default function (done) {
	
	global.button = {
		a: new Monitor({pin: 39, mode: Digital.InputPullUp, edge: Monitor.Rising | Monitor.Falling}),
	};
	button.a.onChanged = nop;
	
	//global.lights = new NeoPixel({});

	done();
}

function nop() {
}
