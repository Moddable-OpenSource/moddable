import Digital from "pins/digital";
import Monitor from "pins/digital/monitor";
import PulseCount from "pins/pulsecount";
import config from "mc/config";

new PulseCount({
		signal: config.jogdial.signal,
		control: config.jogdial.control,
		onReadable() {
			const value = this.read();
			trace(`pulse changed ${this.read()}\n`);

			// wrap around from -20 to +20
			if (value < -20)
				this.write(20);
			else if (value > 20)
				this.write(-20);
		}
});

const monitor = new Monitor({
		pin: config.jogdial.button,
		mode: Digital.InputPullUp,
		edge: Monitor.Falling | Monitor.Rising
});
monitor.onChanged = function() {
	if (this.read())						// Pulled high. Low when pressed.
		trace(`button released\n`);
	else
		trace(`button pressed\n`);
}
