import Serial from "builtin/serial";

const msg = "hello world\r\n";

if (1) {		// using buffers
	let serial = new Serial({
		baud: 115200 * 8,
		onReadable: function(count) {
			this.write(this.read());
		},
	});
	serial.format = "buffer";

	serial.write(ArrayBuffer.fromString(msg));
}
else {		// using bytes
	let serial = new Serial({
		baud: 115200 * 8,
		onReadable: function(count) {
			while (count--)
				this.write(this.read());
		},
	});

	for (let i = 0; i < msg.length; i++)
		serial.write(msg.charCodeAt(i));
}
