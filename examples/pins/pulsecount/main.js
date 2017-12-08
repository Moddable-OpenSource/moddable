import PulseCount from "pins/pulsecount";
import Timer from "timer";

let pulse = new PulseCount({signal: 4, control: 5});
let last = 0;

Timer.repeat(() => {
	let value = pulse.get();
	if (value === last) return;

	last = value;
	trace(value, "\n");

	if (value > 20)
		pulse.set(0);
}, 17);
