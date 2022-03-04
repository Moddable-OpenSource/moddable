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

import Resource from "Resource";

const resource = new Resource("resource.txt");
const bytes = new Uint8Array(resource);
try {
	trace(bytes[0] + "\n");
	bytes[0] = 0;
}
catch (e) {
	trace(e + "\n");
}
const view = new DataView(resource);
try {
	trace(view.getUint8(1) + "\n");
	view.setUint8(1, 1);
}
catch (e) {
	trace(e + "\n");
}
