/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

import Timer from "timer";
import M5PM1 from "embedded:peripheral/Power/M5PM1";
import M5IOE1 from "embedded:peripheral/IOExpander/M5IOE1";
import {setAudioExpander} from "m5stack_stopwatch/audio";

export default function(done) {
	const bus = {...device.i2c.default, io: device.io.SMBus};
	globalThis.power = new M5PM1(bus);

	const Digital = device.io.Digital;
	const cs = new Digital({pin: 39, mode: Digital.Output, initialValue: 1});
	cs.close();
	Timer.delay(50);

	const expander = new M5IOE1(bus);
	expander.write(9, 0);
	expander.write(10, 0);
	for (const pin of [1, 3, 4, 5, 8, 9, 10])
		expander.configure({pin});

	expander.write(8, 0);
	Timer.delay(20);
	for (const pin of [1, 4, 5, 8])
		expander.write(pin, 1);
	expander.write(3, 0);
	expander.write(9, 0);
	expander.write(10, 0);
	Timer.delay(10);

	expander.write(4, 0);
	expander.write(5, 0);
	Timer.delay(8);
	expander.write(4, 1);
	expander.write(5, 1);
	Timer.delay(80);

	setAudioExpander(expander);
	done?.();
}
