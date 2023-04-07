import Digital from "pins/digital";
import {Sleep} from "sleep";
import Timer from "sleep";
import config from "mc/config";

let wakenWith = "";

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
		let digital = new Digital({
			pin: config.lis3dh_int1_pin,
			mode: Digital.Input,		//  mode: Digital.InputPullUp,
			wakeEdge: Digital.WakeOnFall,
			onWake: () => {
			}
		});
		if (duration > 0)
			Sleep.deep(duration);
		else
			Sleep.deep();
	}
	get wakenWith() {
		return wakenWith;
	}
	static setup() {
		const led = new Host.LED.Default;
	}
}
