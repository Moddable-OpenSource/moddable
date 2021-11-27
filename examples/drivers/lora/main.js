import Timer from "timer";
import config from "mc/config";

const interval = 1000;
const transmit = parseInt(config.transmit ?? "0");
if (transmit)
	trace(`Auto-transmit at ${interval} ms inteval\n`);

const buffer = new Uint8Array(250);
let id = 0;

const lora = new device.peripheral.lora.Default({
	onReadable: () => {
		const bytesReceived = lora.read(buffer);

		let s = "";
		for (let l = 0; l < bytesReceived; ++l)
			s += String.fromCharCode(buffer[l]);
		trace(`Received ${bytesReceived} bytes: ${s}\n`);
	},
	onWritable: () => {
		trace("onWritable\n");
	}
});

if (transmit)
	Timer.repeat(doTransmit, interval);

new device.peripheral.button.Flash({
	onPush: function() {
		if (this.pressed)
			doTransmit();
	}
});

function doTransmit() {
	const message = "hello #" + ++id;
	trace(`Sending: ${message}\n`);

	const length = message.length;
	for (let l = 0; l < length; ++l)
		buffer[l] = message.charCodeAt(l);
	lora.write(buffer, length);
 }
