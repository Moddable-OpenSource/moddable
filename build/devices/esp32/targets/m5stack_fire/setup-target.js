import NeoPixel from "neopixel";

import Digital from "pins/digital";
import Monitor from "monitor";

import AudioOut from "pins/audioout";
import Resource from "Resource";

import Timer from "timer";
import MPU6050 from "mpu6050";
import MAG3110 from "mag3110";

import config from "mc/config";

const state = {
	handleRotation: nop
};


export default function (done) {
	global.lights = new NeoPixel({});

	global.button = {
		a: new Monitor({pin: 39, mode: Digital.InputPullUp, edge: Monitor.Rising | Monitor.Falling}),
		b: new Monitor({pin: 38, mode: Digital.InputPullUp, edge: Monitor.Rising | Monitor.Falling}),
		c: new Monitor({pin: 37, mode: Digital.InputPullUp, edge: Monitor.Rising | Monitor.Falling}),
	};
	button.a.onChanged = button.b.onChanged = button.c.onChanged = nop;

	global.speaker = new AudioOut({streams: 4});
	if (config.startupSound) {
		speaker.callback = function() {this.stop()};
		speaker.enqueue(0, AudioOut.Samples, new Resource(config.startupSound));
		speaker.enqueue(0, AudioOut.Callback, 0);
		speaker.start();
	}

	//@@ microphone

	try {
		state.accelerometerGyro = new MPU6050;
		state.magnetometer = new MAG3110;

		global.accelerometer = {
			onreading: nop
		}

		global.gyro = {
			onreading: nop
		}

		global.magnetometer = {
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

		magnetometer.start = function (frequency) {
			magnetometer.stop();
			state.magTimerID = Timer.repeat(id => {
				const sample = state.magnetometer.sample();
				if (sample) {
					magnetometer.onreading(sample);
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

		magnetometer.stop = function () {
			if (undefined !== state.magTimerID)
				Timer.clear(state.magTimerID);
			delete state.magTimerID;
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

	done();
}

function nop() {
}

