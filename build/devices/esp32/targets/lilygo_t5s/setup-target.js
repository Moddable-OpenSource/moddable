import Digital from "pins/digital";
import Monitor from "monitor";

export default function (done) {
	global.button = {
		a: new Monitor({pin: 37, mode: Digital.InputPullUp, edge: Monitor.Rising | Monitor.Falling}),
		b: new Monitor({pin: 38, mode: Digital.InputPullUp, edge: Monitor.Rising | Monitor.Falling}),
		c: new Monitor({pin: 39, mode: Digital.InputPullUp, edge: Monitor.Rising | Monitor.Falling}),
	};
	button.a.onChanged = button.b.onChanged = button.c.onChanged = nop;

	done();
}

function nop() {
}
