import M5Button from "m5button";

import NeoPixel from "neopixel";

export default function (done) {
	globalThis.button = {
		a: new M5Button(39)
	};

	globalThis.lights = new NeoPixel({});

	done();
}
