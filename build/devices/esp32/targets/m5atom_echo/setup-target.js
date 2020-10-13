import M5Button from "m5button";

import NeoPixel from "neopixel";
import AudioOut from "pins/audioout";

import config from "mc/config";

export default function (done) {
	globalThis.button = {
		a: new M5Button(39)
	};

	globalThis.lights = new NeoPixel({});

	if (config.speaker) {
		globalThis.speaker = new AudioOut({
			streams: 2
		});
	}

	done();
}
