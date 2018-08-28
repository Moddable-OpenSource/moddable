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

import Timer from "timer";
import Serial from "pins/serial";

let loopback = new Serial({baudrate: 9600, rx: 33, tx: 32});

const timer = Timer.repeat(id => {
	loopback.write("Echo\n");
	let ret = loopback.read(40);
	trace(String.fromCharCode.apply(String, ret));
}, 1000);

