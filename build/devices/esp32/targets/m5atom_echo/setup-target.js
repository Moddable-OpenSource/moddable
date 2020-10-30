import M5Button from "m5button";

import NeoPixel from "neopixel";
import AudioOut from "pins/audioout";

import Resource from "Resource";
import config from "mc/config";

export default function (done) {
	globalThis.button = {
		a: new M5Button(39)
	};

	globalThis.lights = new NeoPixel({});

	// start-up sound
	if (config.startupSound) {
		const speaker = new AudioOut({streams: 1});
		speaker.callback = function () {
			this.stop();
			this.close();
			this.done();
		};
		speaker.done = done;
		done = undefined;

		speaker.enqueue(0, AudioOut.Samples, new Resource(config.startupSound));
		speaker.enqueue(0, AudioOut.Callback, 0);
		speaker.start();
	}

	done?.();
}
