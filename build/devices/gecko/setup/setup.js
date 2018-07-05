/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

import Timer from "timer";
import Time from "time";

export default function () {
	Timer.set(main);
}

function main() {
	let f = require.weak("main");
	if (typeof f === "function")
		f.call(this);
}

