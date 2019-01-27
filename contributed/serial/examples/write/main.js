import Timer from "timer";
import Serial from "serial";

let serial = new Serial();


let str = "Hello World!";

let buffer = new ArrayBuffer(6);
let chars = new Uint8Array(buffer);

chars[0] = 0x74;
chars[1] = 0x65;
chars[2] = 0x73;
chars[3] = 0x74;
chars[4] = 0x20;
chars[5] = 0x31;


export default function() {
	Timer.repeat(() => {

		serial.write("Array Buffer:\r\n");
		/* array buffer */
		serial.write(buffer);
		serial.write(" -- ");

		/* beginning of array buffer */
		serial.write(buffer, 4);
		serial.write(" -- ");

		/* middle of array buffer */
		serial.write(buffer, 3, 3);
		serial.write(" -- ");

		/* end of array buffer */
		serial.write(buffer, -1);

		serial.write("\r\n");

		serial.write("String:\r\n");
		/* string */
		serial.write(str);
		serial.write(" -- ");

		/* beginning of string */
		serial.write(str, 6);
		serial.write(" -- ");

		/* middle of string */
		serial.write(str, 6, 5);
		serial.write(" -- ");

		/* end of string */
		serial.write(str, -6);

		/* end */
		serial.write("\r\n");
		serial.write("\r\n");
	}, 10000);
}
