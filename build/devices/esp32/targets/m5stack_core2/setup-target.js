import AXP192 from "axp192";
import MPU6886 from "mpu6886";
import AudioOut from "pins/audioout";
import Resource from "Resource";
import Timer from "timer";
import config from "mc/config";

const INTERNAL_I2C_SDA = 21;
const INTERNAL_I2C_SCL = 22;

const state = {
	handleRotation: nop
};

export default function (done) {
	global.power = new AXP192({
		sda: INTERNAL_I2C_SDA,
		scl: INTERNAL_I2C_SCL,
	});

	global.power.setSpeakerEnable(true)
	global.speaker = new AudioOut({streams: 4});
	speaker.callback = function() { this.stop() };
	speaker.enqueue(0, AudioOut.Samples, new Resource("bflatmajor.maud"));
	speaker.enqueue(0, AudioOut.Callback, 0);
	speaker.start();

	state.accelerometerGyro = new MPU6886({
		sda: INTERNAL_I2C_SDA,
		scl: INTERNAL_I2C_SCL
	});

	global.accelerometer = {
		onreading: nop
	}

	global.gyro = {
		onreading: nop
	}

	accelerometer.start = function (frequency) {
		accelerometer.stop();
		state.accelerometerTimerID = Timer.repeat(id => {
			state.accelerometerGyro.configure({
				operation: "accelerometer"
			});
			const sample = state.accelerometerGyro.sample();
			if (sample) {
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
				if (reading.y < -0.7 && application.rotation != 180) {
					application.rotation = 180;
				} else if (reading.y > 0.7 && application.rotation != 0) {
					application.rotation = 0;
				}
			} else {
				if (reading.x < -0.7 && application.rotation != 270) {
					application.rotation = 270;
				} else if (reading.x > 0.7 && application.rotation != 90) {
					application.rotation = 90;
				}
			}
		}
		accelerometer.start(300);
	}
	
	done();
}

function nop() {}
