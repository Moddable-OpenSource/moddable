//import config from "mc/config";
import Timer from "timer";

export default function (done) {
	const Digital = device.io.Digital; 
	globalThis.power = {
		main: new Digital({
			pin: device.pin.powerMain,
			mode: Digital.Output
		}),
		external: new Digital({
			pin: device.pin.powerExternal,
			mode: Digital.Output
		}),
		epd: new Digital({
			pin: device.pin.powerEPD,
			mode: Digital.Output
		}),
	};
	
	power.main.write(1);
	power.external.write(1);
	power.epd.write(1);

	done();
}
