import PulseCount from "pins/pulsecount";
import Timer from "timer";

let pulse = new PulseCount({signal: 6, control: 27});
let last = 0;

pulse.onChanged = function pulseChanged() {
	trace(`pulse changed ${pulse.get()}\n`);
}


Timer.repeat(() => {
	let value = pulse.get();
	if (value === last) return;

	last = value;
	trace(value, "\n");

	if (value > 20)
		pulse.set(0);
}, 17);
