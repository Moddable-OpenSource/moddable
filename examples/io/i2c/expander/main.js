import Expander from "expander";

const expander = new Expander({
	sda: 5,
	scl: 4,
	hz: 1000000,
	interrupt: 0,
	address: 0x20,
});

const input = new expander.Digital({
	pin: 0,
	mode: expander.DigitalBank.InputPullUp,
	edge: expander.Digital.Rising | expander.Digital.Falling,
	onReadable() {
		trace("Interrupt ", this.read(), "\n");
	}
});

const outputs = new expander.DigitalBank({
	pins: 0x00f0,
	mode: expander.DigitalBank.Output,
});

let state = 0;
System.setInterval(() => {
	outputs.write(state);
	state = ~state
}, 250);
