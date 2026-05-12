import config from "mc/config";
import Timer from "timer";

export default function (done) {
	const Digital = device.io.Digital;
	const backlightPin = new Digital({ pin: config.backlight, mode: Digital.Output });
	if ((undefined === config.brightness) || ("none" === config.brightness)) {
		backlightPin.write(0);
		backlightPin.close();
	}
	else if ("off" === config.backlight) {
		backlightPin.write(1);
		backlightPin.close();
	}
	else {
		backlightPin.close();
		globalThis.backlight = new device.peripheral.Backlight;
		backlight.brightness = parseInt(config.brigtness) / 100;
	}
	const displayResetPin = new Digital({ pin: config.lcd_rst_pin, mode: Digital.Output });
	const displayCSPin = new Digital({ pin: config.lcd_cs_pin, mode: Digital.Output });

	displayResetPin.write(1);
	Timer.delay(100);
	displayResetPin.write(0);
	Timer.delay(100);
	displayCSPin.write(0);
	Timer.delay(100);
	displayResetPin.close();
	displayCSPin.close();

	done();
}
