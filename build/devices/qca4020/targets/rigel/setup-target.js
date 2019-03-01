import config from "mc/config";
import Digital from "pins/digital";

export default function (done) {
	if (config.autobacklight)
		Digital.write(config.backlight_pin, 1);

	done();
}
