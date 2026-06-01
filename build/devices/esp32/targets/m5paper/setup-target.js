//import config from "mc/config";
import Timer from "timer";

export default function (done) {
	const Digital = device.io.Digital; 
	globalThis.power = {
		main: new Digital({
			pin: device.pin.powerMain,
			mode: Digital.Output,
			initialValue: 1
		}),
		external: new Digital({
			pin: device.pin.powerExternal,
			mode: Digital.Output,
			initialValue: 1
		}),
		epd: new Digital({
			pin: device.pin.powerEPD,
			mode: Digital.Output,
			initialValue: 1
		}),
	};

	done();
}
