import Serial from "builtin/serial";

const message = ArrayBuffer.fromString("Since publication of the first edition in 1997, ECMAScript has grown to be one of the world's most widely used general-purpose programming languages. It is best known as the language embedded in web browsers but has also been widely adopted for server and embedded applications.\r\n");
let offset = 0;

let serial = new Serial({
	baud: 921600,
	onWritable: function(count) {
		do {
			const use = Math.min(count, message.byteLength - offset);
			serial.write(message.slice(offset, offset + use));
			count -= use;
			offset += use;
			if (offset >= message.byteLength)
				offset = 0;
		} while (count);
	},
});
serial.format = "buffer";
