import Timer from "timer";
import Serial from "serial";

let serial = new Serial();

/* 10 second timeout for reading strings */
serial.setTimeout(10000);

export default function() {
	Timer.repeat(() => {
		let str = serial.readLine();
		while (undefined !== str) {
			trace("got: " + str + "\n");
			serial.writeLine("[" + str + "]");
			str = serial.readLine();
		}
	}, 1000);
}
