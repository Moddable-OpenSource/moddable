import config from "mc/config";
import Timer from "timer";

export default function (done) {
	const Digital = device.io.Digital;

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
