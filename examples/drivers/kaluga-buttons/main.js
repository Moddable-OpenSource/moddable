/*
 * Copyright (c) 2021 Moddable Tech, Inc.
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

const TEST_CALLBACK = 0;
const TEST_PRESSED = 1;
const TEST_READ = 2;

const TEST = TEST_CALLBACK;

let touchButtons = {};
let mechanicalButtons = {};

if (TEST == TEST_CALLBACK) {
	for (let x in Host.Touchpad) {
		touchButtons[x] = new Host.Touchpad[x]({
			onPush: value => {
				trace(`Touch Button ${x}: ${value}\n`);
			}
		})
	}
	for (let x in Host.Button) {
		if (x === "Default") continue;
		mechanicalButtons[x] = new Host.Button[x]({
			onPush: value => {
				trace(`Mechanical Button ${x}: ${value}\n`);
			}
		})
	}
}else{
	let touchValues = {};
	let mechanicalValues = {};

	for (let x in Host.Touchpad) {
		touchButtons[x] = new Host.Touchpad[x]({});
		touchValues[x] = (TEST == TEST_PRESSED) ? false : 0;
	}
	for (let x in Host.Button) {
		if (x === "Default") continue;
		mechanicalButtons[x] = new Host.Button[x]({});
		mechanicalValues[x] = (TEST == TEST_PRESSED) ? false : 0;
	}

	Timer.repeat(id => {
		for (let x in touchButtons) {
			const value = (TEST == TEST_PRESSED) ? touchButtons[x].pressed : touchButtons[x].read();
			if (value !== touchValues[x]) {
				trace(`Touch Button ${x}: ${value}\n`);
				touchValues[x] = value;
			}
		}
		for (let x in mechanicalButtons) {
			const value = (TEST == TEST_PRESSED) ? mechanicalButtons[x].pressed : mechanicalButtons[x].read();
			if (value !== mechanicalValues[x]) {
				trace(`Mechanical Button ${x}: ${value}\n`);
				mechanicalValues[x] = value;
			}
		}
	}, 17);
}

