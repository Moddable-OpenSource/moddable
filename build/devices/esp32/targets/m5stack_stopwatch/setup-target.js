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
import M5PM1 from "m5stack_stopwatch/m5pm1";
import M5IOE1 from "m5stack_stopwatch/m5ioe1";
import {setAudioExpander} from "m5stack_stopwatch/audio";

function findExpanderAddress(bus) {
	const empty = new ArrayBuffer;
	for (const address of [0x4F, 0x6F]) {
		const probe = new device.io.I2C({...bus, address, hz: 100_000});
		try {
			if (probe.write(empty)) {
				// A sleeping M5IOE1 can wake on the first address probe.
				Timer.delay(10);
				if (probe.write(empty))
					continue;
			}
			return address;
		}
		finally {
			probe.close();
		}
	}
	throw new Error("M5IOE1 not found");
}

export default function(done) {
	const bus = {...device.i2c.internal, io: device.io.SMBus};
	const power = new M5PM1(bus);
	let expander;
	try {
		power.configure();

		const Digital = device.io.Digital;
		const cs = new Digital({pin: 39, mode: Digital.Output, initialValue: 1});
		cs.close();
		Timer.delay(50);

		const address = findExpanderAddress(device.i2c.internal);
		expander = new M5IOE1({...bus, address});
		expander.disableIdleSleep();
		expander.write(1, 0);	// Select UART on the rear expansion connector.
		expander.write(9, 0);
		expander.write(10, 0);
		for (const pin of [1, 3, 4, 5, 8, 9, 10])
			expander.configure({pin});

		expander.write(8, 0);
		Timer.delay(20);
		for (const pin of [4, 5, 8])
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
	}
	catch (error) {
		try {
			expander?.close();
		}
		finally {
			power.close();
		}
		throw error;
	}

	globalThis.power = power;
	setAudioExpander(expander);
	done?.();
}
