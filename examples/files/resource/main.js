/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

import Resource from "Resource";

// get access to resource.txt.
// "resource" is a host buffer, which can generally be used like an ArrayBuffer.
// the buffer is stored in flash memory, so it is read-ony and
//	so any attempt to write to it throws an exception.
const resource = new Resource("resource.txt");

// access the resource as an array of bytes
const bytes = new Uint8Array(resource);
try {
	trace(bytes[0] + "\n");		// reading the array succeeds (traces "48")
	bytes[0] = 0;				// writing to the array throws an exception
	// should not reach here
}
catch (e) {
	trace(e + "\n");
}

// access the resource through a DataView
const view = new DataView(resource);
try {
	trace(view.getUint8(1) + "\n");		// reading the data view succeeds (traces "49")
	view.setUint8(1, 1);				// writing to the data view throws an exception
	// should not reach here
}
catch (e) {
	trace(e + "\n");
}
