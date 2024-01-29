import Touch from "embedded:sensor/Touch/FT6x06";
import { SensorStreamMixin } from "sensorstream";

const TouchStream = SensorStreamMixin(Touch);

async function test() {
	const stream = new TouchStream({
		sensor: {
			...device.I2C.default,
			io: device.io.SMBus
		},
		interval: 33
	});
	for await (let point of stream) {
		trace(`point: ${point.id} x: ${point.x} y: ${point.y}\n`);
	}
}
test();
