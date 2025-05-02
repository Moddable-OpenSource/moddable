/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import config from "mc/config";

if (!config.Screen)
	trace("WARNING: no screen configured\n");

export default function (done) {
	if (!global.screen && config.Screen) {
		globalThis.screen = new config.Screen({});

		if (config.driverRotation) {
			if (config.rotation)
				screen.rotation = (config.driverRotation + config.rotation) % 360;
			else
				screen.rotation = config.driverRotation;
		}

		if (globalThis.Host?.Backlight || globalThis.device?.peripheral?.Backlight) {
			let brightness = config.brightness;
			if ((undefined === brightness) || ("none" === brightness))
				brightness = 100;
			else if ("off" === brightness)
				brightness = 0;
			else
				brightness = parseInt(brightness);
			
			if (globalThis.Host?.Backlight)
				globalThis.backlight = new Host.Backlight(brightness);
			else {
				globalThis.backlight = new device.peripheral.Backlight;
				globalThis.backlight.brightness = brightness / 100;
			}
		}
	}

	done();
}
