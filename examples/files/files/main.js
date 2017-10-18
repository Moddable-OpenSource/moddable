/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

/*
 *     09/19/2017 
 *     The SPIFFS file system is required by the file class and this example.
 *     https://github.com/pellepl/spiffs
 */

import {File} from "file";
import {Iterator} from "file";

let file;

// writing/reading strings
file = new File("test.txt", true);
file.write("This is a test.\n");
file.write("We can write ", "multiple", " values.\n");
file.write("This is the end of the test.\n");
file.position = 0;
let content = file.read(String);
trace(content);
file.close();
trace("\n");

// writing/reading JSON
let preferences = { name:"Brian", city:"Del Mar", state:"CA" };
file = new File("preferences.json", true);
file.write(JSON.stringify(preferences));
file.close();
file = new File("preferences.json");
preferences = JSON.parse(file.read(String));
trace(`name: ${preferences.name}, city:${preferences.city}, state:${preferences.state}\n`);
file.close(file)
trace("\n");

// writing/reading ArrayBuffers
let length = 10;
let buffer = new ArrayBuffer(length * 2);
let shorts = new Uint16Array(buffer);
for (let i = 0; i < length; ++i)
	shorts[i] = i;
file = new File("test.bin", true);
file.write(buffer);
trace(`File length: ${file.length}\n`);
file.position = 10;
shorts = new Uint16Array(file.read(ArrayBuffer, 10));
length = shorts.length;
trace("Last five shorts: ");
for (let i = 0; i < length; ++i)
	trace(shorts[i] + ' ');
trace("\n");

// directory iterator
// Note: The SPIFFS file system used on the ESP8266 is a flat file system with no directories
let root = new Iterator("/");
let item;
while (item = root.next()) {
	if (undefined == item.length)
		trace(`Directory: ${item.name}\n`);
	else
		trace(`File: ${item.name}, ${item.length} bytes\n`);
}
trace("\n");

File.delete("test.txt");
File.delete("preferences.json");
File.delete("test.bin");

