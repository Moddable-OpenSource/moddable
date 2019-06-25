import Digital from "pins/digital";
import Monitor from "monitor";
import AXP192 from "axp192";

export default function (done) {
	global.button = {
		a: new Monitor({pin: 37, mode: Digital.InputPullUp, edge: Monitor.RisingEdge | Monitor.FallingEdge}),
		b: new Monitor({pin: 39, mode: Digital.InputPullUp, edge: Monitor.RisingEdge | Monitor.FallingEdge}),
	};
	button.a.onChanged = button.b.onChanged = nop;

	global.power = new AXP192({
		sda: 21,
		scl: 22,
		address: 0x34
	});

	//@@ microphone

	done();
}

function nop() {
}
