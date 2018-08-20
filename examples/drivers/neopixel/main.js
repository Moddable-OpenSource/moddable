import NeoPixel from "neopixel";
import Timer from "timer";

const np = new NeoPixel({length: 19, pin: 22, order: "GRB"});

Timer.delay(1);
np.fill(np.makeRGB(255, 255, 255)); np.update();
Timer.delay(500);
np.fill(np.makeRGB(255, 0, 0)); np.update();
Timer.delay(500);
np.fill(np.makeRGB(0, 255, 0)); np.update();
Timer.delay(500);
np.fill(np.makeRGB(0, 0, 255)); np.update();
Timer.delay(500);

let value = 0x01;
Timer.repeat(() => {
	let v = value;
	for (let i = 0; i < np.length; i++) {
		v <<= 1;
		if (v == (1 << 24))
			v = 1;
		np.setPixel(i, v);
	}

	np.update();

	value <<= 1;
	if (value == (1 <<24))
		value = 1;
}, 33);
