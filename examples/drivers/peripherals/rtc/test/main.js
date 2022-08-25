/*
 * Copyright (c) 2021-2022 Moddable Tech, Inc.
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
import RTC from "embedded:RTC/DS1307";

const rtc = new RTC({
            clock: {
                ...device.I2C.default,
                io: device.io.SMBus
            }
});

trace(`Today's date: ${new Date()}\n`);

if (rtc.time === undefined)
	trace(`CLOCK NOT ENABLED\n`);

let test = 1568045349000; 			// GMT Mon Sep 09 2019 16:09:09 GMT+0000

trace(`set test date(${test})\nCompare  [js]:(${test}) ${(new Date(test)).toGMTString()}\n`);
rtc.time = test;
let now = rtc.time;
trace(`Compare [rtc]:(${now}) ${(new Date(now)).toGMTString()}\n`);

let testset = [
	'1995-12-17T03:24:00',
	'1996-01-02T03:24:00',
	'2007-09-08T07:06:00',
	'2021-03-11T01:02:00' ];

for (let i=0; i<testset.length; i++) {
	let v = (new Date(testset[i])).getTime();
	trace(`set  javascript: date(${v}): ${(new Date(v)).toGMTString()}\n`);
	try {
		rtc.time = v;
		trace(`wait ${i} seconds\n`);
		Timer.delay(i * 1000);
		let t = rtc.time;
		trace(`read from   RTC: date(${t}): ${(new Date(t)).toGMTString()}\n`);
	}
	catch (e) {
		trace(`Error setting time.\n`);
	}
	trace(`---------------\n`);
}

trace(`close rtc.\n`);
rtc.close();

