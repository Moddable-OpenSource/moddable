import M5Button from "m5button";
import config from "mc/config";
import Timer from "timer";

class PowerButton {
	#value = 0;

	read() {
		return this.#value;
	}
	write(value) {
		if (this.#value === value)
			return;
		this.#value = value;
		this.onChanged?.();
	}
}

export default function (done) {
	globalThis.button = {
		a: new M5Button(37),
		b: new M5Button(39)
	};

	globalThis.power = new device.peripheral.Power();

	if (config.enablePowerButton) {
		globalThis.button.power = new PowerButton();
		// AXP192 PEK reports latched press events, so expose them as a short 1 -> 0 pulse.
		Timer.repeat(() => {
			const state = globalThis.power.getPekState();
			globalThis.button.power.write(state ? 1 : 0);
		}, 100);
	}
	
	if (config.autorotate && globalThis.Application) {
		const imu = new device.sensor.IMU();
		Timer.repeat(id => {
			const sample = imu.sample();
			const {x, y } = sample.accelerometer;
			if (Math.abs(y) > Math.abs(x)) {
				if (y < -0.7 && application.rotation !== 270) {
					application.rotation = 270;
				} else if (y > 0.7 && application.rotation !== 90) {
					application.rotation = 90;
				}
			} else {
				if (x < -0.7 && application.rotation !== 180) {
					application.rotation = 180;
				} else if (x > 0.7 && application.rotation !== 0) {
					application.rotation = 0;
				}
			}
		}, 300);
	}

	done();
}
