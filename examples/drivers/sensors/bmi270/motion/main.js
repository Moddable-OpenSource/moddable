/*
 * Copyright (c) 2026 Satoshi Tanaka
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import BMI270 from "embedded:sensor/Accelerometer-Gyroscope-Magnetometer/BMI270";
import Timer from "timer";

const onSignificantMotion = () => {
	trace(`onSignificantMotion\n`)
}
const onStepCount = (stepCount) => {
	trace(`onStepCount: ${stepCount}\n`)
}

const Activity = [
	"still",
	"walking",
	"running",
	"unknown"
];
const onActivityChange = (act) => {
	trace(`onActivityChange: ${Activity[act] ?? act}\n`);
}

const onWristWakeup = (gest) => {
	trace(`onWristWakeup: ${gest}\n`)
}

const WristGesture = [
	"unknown",
	"push arm down",
	"pivot up",
	"wrist shake/jiggle",
	"flick in",
	"flick out",
	"reserved",
	"reserved"
];
const onWristGesture = (gest) => {
	trace(`onWristGesture: ${WristGesture[gest] ?? gest}\n`);
}
const onNoMotion = () => {
	trace(`onNoMotion\n`)
}

const onAnyMotion = () => {
	trace(`onAnyMotion\n`)
}

const sensor = new BMI270({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	},
	onSignificantMotion,
	onStepCount,
	onActivityChange,
	onWristWakeup,
	onWristGesture,
	onNoMotion,
	onAnyMotion
});

sensor.configure({
	latched: 1,
	anyMotion: {
		enable: true,
		axis: 0b111,
		duration: 3,
		threshold: 20
	},
	noMotion: {
		enable: true,
		axis: 0b111,
		duration: 50,
		threshold: 20
	},
	significantMotion: {
		enable: true,
		blockSize: 0
	},
	stepCounter: {
		enable_detector: true,
		enable_counter: true,
		enable_activity: true,
		watermark_level: 1
	},
	activityChange: {
		enable: true
	},
	wristWakeup: {
		enable: true
	},
	wristGesture: {
		enable: true
	}
});

Timer.repeat(() => {
	sensor.sample()
}, 100);
