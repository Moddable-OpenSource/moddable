/*
 * Copyright (c) 2020-2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import NeoPixel from "neopixel";

import AudioOut from "pins/audioout";
import Resource from "Resource";

import Timer from "timer";
import MPU6050 from "mpu6050";
import MAG3110 from "mag3110";
import M5Button from "m5button";

import config from "mc/config";

const state = {
	handleRotation: nop
};

export default function (done) {
	Object.defineProperty(globalThis, "lights", {
		enumerable: true,
		configurable: true,
		get() {		// instantiate lights on first access
			const value = new NeoPixel({}); 
			Object.defineProperty(globalThis, "lights", {
				enumerable: true,
				configurable: true,
				writable: true,
				value
			});
			return value;
		}
	});

	globalThis.button = {
		a: new M5Button(39),
		b: new M5Button(38),
		c: new M5Button(37)
	};

	// start-up sound
	if (config.startupSound) {
		const speaker = new AudioOut({streams: 1});
		speaker.callback = function () {
			this.stop();
			this.close();
			Timer.set(this.done);
		};
		speaker.done = done;
		done = undefined;

		speaker.enqueue(0, AudioOut.Samples, new Resource(config.startupSound));
		speaker.enqueue(0, AudioOut.Callback, 0);
		speaker.start();
	}

	try {
		state.accelerometerGyro = new MPU6050;
		state.magnetometer = new MAG3110;

		globalThis.accelerometer = {
			onreading: nop
		}

		globalThis.gyro = {
			onreading: nop
		}

		globalThis.magnetometer = {
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

	if (config.autorotate && globalThis.Application && globalThis.accelerometer) {
		state.handleRotation = function (reading) {
			if (globalThis.application === undefined) return;

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

	done?.();
}

function nop() {
}

