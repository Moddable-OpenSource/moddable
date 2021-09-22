import Digital from "pins/digital";
import config from "mc/config";

export default function (done) {
	if ((undefined === config.brightness) || ("none" === config.brightness))
		Digital.write(config.backlight, 0);
	else if ("off" === config.brightness)
		Digital.write(config.backlight, 1);
	else {
		globalThis.backlight = new device.peripheral.Backlight;
		backlight.brightness = parseInt(config.brightness) / 100;
	}

	done();
}
