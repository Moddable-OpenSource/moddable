/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

import {File, Iterator} from "file";

//const root = "/Users/hoddie/";
//const root = "c:\\Users\\brianfriedkin\\";
const root = "/";
let file;

// writing/reading strings
file = new File(root + "test.txt", true);
file.write("This is a test.\n");
file.write("We can write ", "multiple", " values.\n");
file.write("This is the end of the test.\n");
file.position = 0;
let content = file.read(String);
trace(content);
file.close();
trace("\n");

// writing/reading JSON
let preferences = { name: "Brian", city: "Del Mar", state: "CA" };
file = new File(root + "preferences.json", true);
file.write(JSON.stringify(preferences));
file.close();
file = new File(root + "preferences.json");
preferences = JSON.parse(file.read(String));
trace(`name: ${preferences.name}, city: ${preferences.city}, state: ${preferences.state}\n`);
file.close(file)
trace("\n");

// writing/reading ArrayBuffers
let length = 10;
let buffer = new ArrayBuffer(length * 2);
let shorts = new Uint16Array(buffer);
for (let i = 0; i < length; ++i)
	shorts[i] = i;
file = new File(root + "test.bin", true);
file.write(buffer);
trace(`File length: ${file.length}\n`);
file.position = 10;
shorts = new Uint16Array(file.read(ArrayBuffer, 10));
length = shorts.length;
trace("Last five shorts: ");
for (let i = 0; i < length; ++i)
	trace(shorts[i] + ' ');
file.close(file)
trace("\n");

// directory iterator
// Note: The SPIFFS file system used on the ESP8266 is a flat file system with no directories
let iterator = new Iterator(root);
let item;
while (item = iterator.next()) {
	if (undefined == item.length)
		trace(`${item.name.padEnd(32)} directory\n`);
	else
		trace(`${item.name.padEnd(32)} file          ${item.length} bytes\n`);
}
trace("\n");

File.delete(root + "test.txt");
File.delete(root + "preferences.json");
File.delete(root + "test.bin");
