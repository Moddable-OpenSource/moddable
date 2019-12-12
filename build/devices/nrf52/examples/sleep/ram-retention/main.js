/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
	This application demonstrates how to retain a buffer in RAM across System Off power saving mode (deep sleep).
	The device is woken up from a digital input.
	Upon wakeup, the application re-launches and the reset reason is traced to the console.
	Press the button connected to the digital input pin to wakeup the device.
*/

import {Sleep, ResetReason} from "sleep";
import Timer from "timer";

const PIN = 25;		// Button 4 on nRF52840-DK

let str = valueToString(ResetReason, Sleep.resetReason);

trace(`Good morning. Reset reason: ${str}\n`);

// Check if retained ram buffer is available
let retained;
let buffer = Sleep.retainedRAMBuffer;
if (undefined !== buffer) {
	retained = new Uint8Array(buffer);
	for (let i = 0; i < 100; ++i)
		if (retained[i] != i)
			throw new Error("corrupt retention buffer");
			
	trace(`Retention buffer read and okay.\n`);
}

// No retained ram buffer is available. Retain a buffer and sleep.
else {
	retained = new Uint8Array(100);
	for (let i = 0; i < 100; ++i)
		retained[i] = i;
		
	let count = 6;
	Timer.repeat(id => {
		if (0 == count) {
			Timer.clear(id);
		debugger;
			Sleep.retainedRAMBuffer = retained.buffer;
			Sleep.wakeOnDigital(PIN);
			Sleep.deep();
		}
		if (count > 1)
			trace(`Going to deep sleep in ${count - 1} seconds...\n`);
		else
			trace(`Good night. Press the button connected to pin ${PIN} to wake me up.\n\n`);
		--count;
	}, 1000);
}


function valueToString(obj, value) {
	let result = Object.keys(obj).find(element => obj[element] == value);
	return (result ? result : "Unknown");
}
