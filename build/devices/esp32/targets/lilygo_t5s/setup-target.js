import Digital from "pins/digital";
import Monitor from "monitor";

export default function (done) {
	global.button = {
		a: new Monitor({pin: 37, mode: Digital.InputPullUp, edge: Monitor.RisingEdge | Monitor.FallingEdge}),
		b: new Monitor({pin: 38, mode: Digital.InputPullUp, edge: Monitor.RisingEdge | Monitor.FallingEdge}),
		c: new Monitor({pin: 39, mode: Digital.InputPullUp, edge: Monitor.RisingEdge | Monitor.FallingEdge}),
	};
	button.a.onChanged = button.b.onChanged = button.c.onChanged = nop;

	done();
}

function nop() {
}
