import NeoPixel from "neopixel";

import Digital from "pins/digital";
import Monitor from "monitor";

import AudioOut from "pins/i2s";
import Resource from "Resource";

export default function (done) {
	global.lights = new NeoPixel({});

	global.button = {
		a: new Monitor({pin: 39, mode: Digital.InputPullUp, edge: Monitor.RisingEdge | Monitor.FallingEdge}),
		b: new Monitor({pin: 38, mode: Digital.InputPullUp, edge: Monitor.RisingEdge | Monitor.FallingEdge}),
		c: new Monitor({pin: 37, mode: Digital.InputPullUp, edge: Monitor.RisingEdge | Monitor.FallingEdge}),
	};
	button.a.onChanged = button.b.onChanged = button.c.onChanged = nop;

	global.speaker = new AudioOut({streams: 4});
	speaker.callback = function() {this.stop()};
	speaker.enqueue(0, AudioOut.Samples, new Resource("bflatmajor.maud"));
	speaker.enqueue(0, AudioOut.Callback, 0);
	speaker.start();

	//@@ microphone

	done();
}

function nop() {
}

