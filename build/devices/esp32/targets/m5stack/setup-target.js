import Digital from "pins/digital";
import Monitor from "monitor";

import AudioOut from "pins/audioout";
import Resource from "Resource";

class Button extends Monitor {
	constructor(pin) {
		super({pin, mode: Digital.InputPullUp, edge: Monitor.RisingEdge | Monitor.FallingEdge});
		this.onChanged = this.nop;
	}
	nop() {
	}
}

export default function (done) {
	global.button = {
		a: new Button(39),
		b: new Button(38),
		c: new Button(37),
	};

	global.speaker = new AudioOut({streams: 4});
	speaker.callback = function() {this.stop()};
	speaker.enqueue(0, AudioOut.Samples, new Resource("bflatmajor.maud"));
	speaker.enqueue(0, AudioOut.Callback, 0);
	speaker.start();

	//@@ microphone

	done();
}
