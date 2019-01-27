import Timer from "timer";
import Serial from "serial";

let serial = new Serial();

/* 10 second timeout for reading strings */
serial.setTimeout(10000);

let arrayBuffer = new ArrayBuffer(64);
let values = new Uint8Array(arrayBuffer);


export default function() {
	Timer.repeat(() => {
		serial.write("Send some data please\r\n");
		let amt = serial.readBytesUntil(arrayBuffer, "mn");
		if (amt > 0) {
			trace("got: ");
			for (let i=0; i<amt; i++) {
				trace(`${values[i]} `);
			}
			trace("\n");
		}
	}, 1000);
}
