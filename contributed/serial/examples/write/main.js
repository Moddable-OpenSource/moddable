/*
 * Copyright (c) 2019  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */
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
