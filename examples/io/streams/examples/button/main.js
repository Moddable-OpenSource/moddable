const Digital = device.io.Digital;
import { TransformStream } from "streams";
import { IOReadableStreamMixin, IOWritableStreamMixin } from "iostreams";

const DigitalReadableStream = IOReadableStreamMixin(Digital);
const DigitalWritableStream = IOWritableStreamMixin(Digital);

const writable = new DigitalWritableStream({
	pin: device.pin.led,
	mode: Digital.Output,
});

const readable = new DigitalReadableStream({
	pin: device.pin.button,
	mode: Digital.InputPullUp,
	edge: Digital.Rising | Digital.Falling,
});

const invert = new TransformStream({
	transform(state, controller) {
		controller.enqueue(state ^ 1);
	}
});

readable.pipeThrough(invert).pipeTo(writable);