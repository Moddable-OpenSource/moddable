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
/*
	This application demonstrates how to use a combination of Analog and Digital objects to trigger wakeup from deep sleep.
	Upon wakeup, the application re-launches, blinks the led, and displays the wakeup source.
*/

import Analog from "pins/analog";
import Digital from "pins/digital";
import {Sleep, ResetReason} from "sleep";
import Timer from "timer";
import parseBMF from "commodetto/parseBMF";
import Poco from "commodetto/Poco";
import Resource from "Resource";

let render = new Poco(screen);
let black = render.makeColor(0, 0, 0);
let white = render.makeColor(255, 255, 255);
let font = parseBMF(new Resource("OpenSans-Semibold-28.bf4"));

const led = new Host.LED.Default;
led.write(1);
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
render.end();

let digital1 = new Digital({
	pin: 13,
	mode: Digital.InputPullUp,
	wakeEdge: Digital.WakeOnFall,
	onWake() {
		notify("BackBtn");
	}
});

let digital2 = new Digital({
	pin: 11,
	mode: Digital.InputPullUp,
	wakeEdge: Digital.WakeOnFall,
	onWake() {
		notify("JogPress");
	}
});

let analog = new Analog({
	pin: 5,
	wakeValue: 512,
	wakeCrossing: Analog.CrossingUpDown,
	onWake() {
		notify("analog");
	}
});

switch (Sleep.resetReason) {
	case ResetReason.RESETPIN:
		notify("ResetBtn");
		break;
	case ResetReason.GPIO:
		notify("GPIO");
		break;
	case ResetReason.LPCOMP:
		notify("LPCOMP");
		break;
	case ResetReason.SREQ:
		notify("SREQ");
		break;
}

Timer.set(() => {
	led.write(0);
	notify("sleeping", false);
	Sleep.deep();
}, 3000);

function notify(text, blink = true) {
	render.begin();
		render.fillRectangle(black, 0, 0, render.width, render.height);
		render.drawText(text, font, white,
			(render.width - render.getTextWidth(text, font)) >> 1,
			(render.height - font.height) >> 1);
	render.end();
	if (blink) {
		for (let i = 0; i < 10; ++i) {
			led.write(1);
			Timer.delay(50);
			led.write(0);
			Timer.delay(50);
		}
	}
}
