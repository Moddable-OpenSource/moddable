import Timer from "timer";
import Digital from "pins/digital";
import Monitor from "monitor";
import PulseCount from "pins/pulsecount";
import config from "mc/config";

let pulse = new PulseCount({signal: config.click_wheel_a, control: config.click_wheel_b});
let last = 0;

pulse.onChanged = function() {
	trace(`pulse changed ${pulse.get()}\n`);
}

let monitor = new Monitor({pin: config.click_wheel_sw, mode: Digital.InputPullUp, edge: Monitor.Falling | Monitor.Rising});

monitor.onChanged = function() {
	if (this.read())						// pulled high. Low when pressed.
		trace(`click_wheel released\n`);
	else
		trace(`click_wheel pressed\n`);
}

Timer.repeat(() => {
	let value = pulse.get();
	if (value === last) return;

	last = value;
	trace(value, "\n");

	if (value > 20)
		pulse.set(0);
}, 17);
