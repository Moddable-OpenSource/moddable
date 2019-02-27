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

import ArduCAM from "arducam"
import ILI9341 from "ili9341";
import Bitmap from "commodetto/Bitmap";
import Timer from "timer";

//ArduCAM.disable();

let camera = new ArduCAM({width: 320, height: 240, format: "rgb565be"});

let display = new ILI9341({width: 240, height: 320, pixelFormat: Bitmap.RGB565BE});

let pixels = new ArrayBuffer(320 * 2);

Timer.repeat(id => {
	let bytes = camera.capture();
	trace(`ArduCAM - ${bytes} bytes captured\n`);

	let y = 0;
	while (0 != camera.read(pixels)) {
		display.begin(0, y++, 240, 1);
		display.send(pixels, 40 * 2, 240 * 2);
		display.end();
	}
}, 1);
