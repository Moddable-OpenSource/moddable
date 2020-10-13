import Digital from "pins/digital";
import Monitor from "monitor";

import AudioOut from "pins/audioout";
import Resource from "Resource";
import config from "mc/config";

class Button extends Monitor {
	constructor(pin) {
		super({pin, mode: Digital.InputPullUp, edge: Monitor.Rising | Monitor.Falling});
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

	global.speaker = new AudioOut({streams: 4, });
	if (config.startupSound) {
		speaker.callback = function() {this.stop()};
		speaker.enqueue(0, AudioOut.Samples, new Resource(config.startupSound));
		speaker.enqueue(0, AudioOut.Callback, 0);
		speaker.start();
	}

	//@@ microphone

	done();
}
