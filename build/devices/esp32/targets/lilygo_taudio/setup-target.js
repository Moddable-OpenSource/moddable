import config from "mc/config";

import NeoPixel from "neopixel";

import Digital from "pins/digital";
import Monitor from "monitor";

import WM8978 from "wm8978";

export default function (done) {
	global.lights = new NeoPixel({});

	global.button = {
		a: new Monitor({pin: 35, mode: Digital.InputPullUp, edge: Monitor.RisingEdge | Monitor.FallingEdge}),
		b: new Monitor({pin: 34, mode: Digital.InputPullUp, edge: Monitor.RisingEdge | Monitor.FallingEdge}),
		c: new Monitor({pin: 39, mode: Digital.InputPullUp, edge: Monitor.RisingEdge | Monitor.FallingEdge}),
	};
	button.a.onChanged = button.b.onChanged = button.c.onChanged = nop;

	// microphone
	new WM8978(config.WM8978);	// initialize I2S part



	done();
}

function nop() {}

