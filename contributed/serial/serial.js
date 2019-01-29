/*
 * Copyright (c) 2018-2019  Moddable Tech, Inc.
 *        
 *   This file is part of the Moddable SDK Tools.
 *      
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */
class Serial @ "xs_serial_destructor" {
	constructor(dictionary) @ "xs_serial";

	/* set read or write timeout in ms */
	setTimeout(timeout) @ "xs_serial_setTimeout";

	/* set baudrate (to change after intial configuration) */
	setBaudrate(baud) @ "xs_serial_setBaudrate";

	/* is data available to read */
	available() @ "xs_serial_available";

	/* flush read buffer */
	flush() @ "xs_serial_flush";

	/* str = serial.readBytes(num) */
	/*    read a number of bytes; return a string  */
	/* num = serial.readBytes(buffer, len); */
	/*    read a number of bytes into an array buffer; return bytes read */
	/*    len (optional) is used to limit the length. */
	readBytes(params) @ "xs_serial_readBytes";

	/* read a number of bytes, return if one of the terminator characters is hit or the number of bytes has been received */
	/* readBytesUntil(terminators, len) will return a string */
	/* readBytesUntil(bufer, terminators, len) will fill the array buffer */
	readBytesUntil(params) @ "xs_serial_readBytesUntil";

	/* read a string (cr or lf terminated), up to 256 characters */
	readLine() {
		return this.readBytesUntil("\r\n", 256);
	}

	/* read a string (cr, lf, or other arbitrary termination characters */
	readLineUntil(terminator) {
		return this.readBytesUntil("\r\n"+terminator);
	}

	/* write a string, or starting substring */
	/* serial.write(string);  or serial.write(string, length); */
	/* alternately, write an ArrayBuffer */
	write() @ "xs_serial_write";

	/* write a cr/lf terminated string */
	writeLine(string) {
		return this.write(string + "\r\n");
	}

	/* set up a .onDataReceived(data, len) function and then call poll() */
	/* for asynchronous serial read handling							 */
	/*		serial.onDataReceived = function(data, len) { }				*/
	/* call poll with no parameters to stop polling						*/
	/*		serial.poll();												*/
	/* poll takes a dictionary of parameters.							*/
	/* 		serial.poll({interval:1000,									*/
	/*					terminators:"\r\n",								*/
	/*					trim: 1,										*/
	/*					chunkSize: 64									*/
	/*					});												*/
	poll() @ "xs_serial_poll";

};


Object.freeze(Serial.prototype);

export default Serial;

