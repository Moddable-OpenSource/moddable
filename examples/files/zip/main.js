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
 *     Note: The ZIP implementation requires all files in the ZIP file to be uncompressed.
 *     The following command line creates a ZIP file named test.zip with the uncompressed
 *     contents of the directory test:
 *     
 *     zip -0r test.zip test
 */
 
import {ZIP} from "zip";
import Resource from "Resource";

let buffer = new Resource("test.zip");
let archive = new ZIP(buffer);

function indent(level) {
	trace('..'.repeat(level));
}

function iterateZipDirectory(directory, level) {
	let item, root = archive.iterate(directory);
	indent(level);
	trace(`Iterating directory: ${directory}\n`);
	while (item = root.next()) {
		if (undefined == item.length) {
			trace(`Directory: ${item.name}\n`);
			iterateZipDirectory(directory + item.name + "/", level + 1);
		}
		else {
			indent(level);
			trace(`File: ${item.name}, ${item.length} bytes\n`);
		}
	}
}

// Iterate files in zip archive
iterateZipDirectory("test/", 0);

// Read file in archive
let file = archive.file("test/file0.txt");
let string = file.read(String);
trace(string + "\n");
file.close();
