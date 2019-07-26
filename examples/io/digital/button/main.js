import Digital from "builtin/digital";

let led = new Digital({
   pin: 2,
   mode: Digital.Output,
});
led.write(1);		// off

let button = new Digital({
	pin: 0,
	mode: Digital.InputPullUp,
	edge: Digital.Rising | Digital.Falling,
	onReadable() {
		led.write(this.read());
	}
});
