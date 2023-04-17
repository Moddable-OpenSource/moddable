import Digital from "pins/digital";
import {Sleep} from "sleep";
import Timer from "sleep";
import config from "mc/config";

let wakenWith = "timer";

export default class {
	constructor() {
		if (Sleep.getLatch(config.lis3dh_int1_pin))
			wakenWith = "accelerometer";
	}
	getRetainedValue(index) {
		return Sleep.getRetainedValue(index);
	}
	setRetainedValue(index, value) {
		Sleep.setRetainedValue(index, value);
	}
	sleep(duration) {
		if (duration > 0) {
			let digital = new Digital({
				pin: config.lis3dh_int1_pin,
				mode: Digital.Input,		//  mode: Digital.InputPullUp,
				wakeEdge: Digital.WakeOnFall,
				onWake: () => {
				}
			});
			Sleep.deep(duration);
		}
		else {
			let digital1 = new Digital({
				pin: config.button1_pin,
				mode: Digital.InputPullUp,
				wakeEdge: Digital.WakeOnFall,
				onWake() {
					wakenWith = "button";
				}
			});

			let digital2 = new Digital({
				pin: config.jogdial.button,
				mode: Digital.InputPullUp,
				wakeEdge: Digital.WakeOnFall,
				onWake() {
					wakenWith = "jogdial";
				}
			});
			Sleep.deep();
		}
	}
	get wakenWith() {
		return wakenWith;
	}
	static setup() {
		const led = new Host.LED.Default;
	}
}
