// import Digital from "pins/digital";
// import Monitor from "monitor";
import AXP192 from "axp192";

import AudioOut from "pins/audioout";
import Resource from "Resource";
import PWM from "pins/pwm"
import Digital from "pins/digital"

import Timer from "timer";
import MPU6050 from "mpu6050";
import config from "mc/config";

const state = {
	handleRotation: nop
};

class Backlight extends PWM {
	constructor() {
		super({pin: config.backlight});
		this.write(parseInt(config.brightness));
	}
	write(value) {
		value = 100 - parseInt(value);		// PWM is inverted
		if (value < 0)
			value = 0;
		else if (value > 100)
			value = 100;
		super.write((value / 100) * 1023);
	}
}

export default function (done) {
	if ("none" !== config.brightness) {
		globalThis.backlight = new Backlight;
	} else {
		Digital.write(config.backlight, 0);
	}

	global.speaker = new AudioOut({streams: 4});
	speaker.callback = function() {this.stop()};
	speaker.enqueue(0, AudioOut.Samples, new Resource("bflatmajor.maud"));
	speaker.enqueue(0, AudioOut.Callback, 0);
	speaker.start();

	global.power = new AXP192({
		address: 0x34
	});
	global.power.setBrightness(10)

	//@@ microphone

	/*
	try {
		state.accelerometerGyro = new MPU6050;

		global.accelerometer = {
			onreading: nop
		}

		global.gyro = {
			onreading: nop
		}

		accelerometer.start = function(frequency){
			accelerometer.stop();
			state.accelerometerTimerID = Timer.repeat(id => {
				state.accelerometerGyro.configure({ operation: "accelerometer" });
				const sample = state.accelerometerGyro.sample();
				if (sample) {
					sample.y *= -1;
					sample.z *= -1;
					state.handleRotation(sample);
					accelerometer.onreading(sample);
				}
			}, frequency);
		}

		gyro.start = function(frequency){
			gyro.stop();
			state.gyroTimerID = Timer.repeat(id => {
				state.accelerometerGyro.configure({ operation: "gyroscope" });
				const sample = state.accelerometerGyro.sample();
				if (sample) {
					let {x, y, z} = sample;
					const temp = x;
					x = y * -1;
					y = temp * -1;
					z *= -1;
					gyro.onreading({ x, y, z });
				}
			}, frequency);
		}

		accelerometer.stop = function(){
			if (undefined !== state.accelerometerTimerID)
				Timer.clear(state.accelerometerTimerID);
			delete state.accelerometerTimerID;
		}

		gyro.stop = function () {
			if (undefined !== state.gyroTimerID)
				Timer.clear(state.gyroTimerID);
			delete state.gyroTimerID;
		}
	}
	catch (e) {
		trace(`Error initializing: ${e}\n`);
	}

	if (config.autorotate && global.Application && global.accelerometer) {
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
	*/

	done();
}

function nop() {
}

