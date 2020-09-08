import Digital from "pins/digital";
import Monitor from "monitor";
import AXP192 from "axp192";
import SH200Q from "sh200q";
import MPU6886 from "mpu6886";
import I2C from "pins/i2c";
import Timer from "timer";

import config from "mc/config";

const state = {
	handleRotation: nop
};

export default function (done) {
	global.button = {
		a: new Monitor({
			pin: 37,
			mode: Digital.InputPullUp,
			edge: Monitor.Rising | Monitor.Falling
		}),
		b: new Monitor({
			pin: 39,
			mode: Digital.InputPullUp,
			edge: Monitor.Rising | Monitor.Falling
		}),
	};
	button.a.onChanged = button.b.onChanged = nop;

	global.power = new AXP192({
		sda: 21,
		scl: 22,
	});

	state.accelerometerGyro = new IMU;

	global.accelerometer = {
		onreading: nop
	}

	global.gyro = {
		onreading: nop
	}

	//trace('The Temp:', state.accelerometerGyro.sampleTemp(), '\n');

	accelerometer.start = function (frequency) {
		accelerometer.stop();
		state.accelerometerTimerID = Timer.repeat(id => {
			state.accelerometerGyro.configure({
				operation: "accelerometer"
			});
			const sample = state.accelerometerGyro.sample();
			if (sample) {
				sample.y *= -1;
				sample.z *= -1;
				state.handleRotation(sample);
				accelerometer.onreading(sample);
			}
		}, frequency);
	}

	gyro.start = function (frequency) {
		gyro.stop();
		state.gyroTimerID = Timer.repeat(id => {
			state.accelerometerGyro.configure({
				operation: "gyroscope"
			});
			const sample = state.accelerometerGyro.sample();
			if (sample) {
				let {
					x,
					y,
					z
				} = sample;
				const temp = x;
				x = y * -1;
				y = temp * -1;
				z *= -1;
				gyro.onreading({
					x,
					y,
					z
				});
			}
		}, frequency);
	}

	accelerometer.stop = function () {
		if (undefined !== state.accelerometerTimerID)
			Timer.clear(state.accelerometerTimerID);
		delete state.accelerometerTimerID;
	}

	gyro.stop = function () {
		if (undefined !== state.gyroTimerID)
			Timer.clear(state.gyroTimerID);
		delete state.gyroTimerID;
	}

	if (config.autorotate && global.Application) {
		state.handleRotation = function (reading) {
			if (Math.abs(reading.y) > Math.abs(reading.x)) {
				if (reading.y < -0.7 && application.rotation != 90) {
					application.rotation = 90;
				} else if (reading.y > 0.7 && application.rotation != 270) {
					application.rotation = 270;
				}
			} else {
				if (reading.x < -0.7 && application.rotation != 180) {
					application.rotation = 180;
				} else if (reading.x > 0.7 && application.rotation != 0) {
					application.rotation = 0;
				}
			}
		}
		accelerometer.start(300);
	}

	done();
}

function nop() {}

class IMU {
	constructor() {
		const probe = new I2C({
			address: 0x68,		// MPU6886
			throw: false
		});
		const result = probe.write(0x75);
		probe.close();

		return (result instanceof Error) ? new SH200Q : new MPU6886;
	}
}
