import Digital from "pins/digital";
import config from "mc/config";
import Timer from "timer";


export default function (done) {
	if (undefined !== config.backlight)
		Digital.write(config.backlight, 1);

	Digital.write(config.lcd_rst_pin, 1);
	Timer.delay(100);
	Digital.write(config.lcd_rst_pin, 0);
	Timer.delay(100);
	Digital.write(config.lcd_rst_pin, 0);
	Digital.write(config.lcd_cs_pin, 0);
	Timer.delay(100);

	done();
}
