import config from "mc/config";
import Timer from "timer";

export default function (done) {
	const Digital = device.io.Digital;

	const displayResetPin = new Digital({ pin: config.lcd_rst_pin, mode: Digital.Output, initialValue: 1 });
	const displayCSPin = new Digital({ pin: config.lcd_cs_pin, mode: Digital.Output, initialValue: 0 });

	Timer.delay(100);
	displayResetPin.write(0);
	Timer.delay(200);
	displayResetPin.close();
	displayCSPin.close();

	done();
}
