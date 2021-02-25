/*
 * Copyright (c) 2021  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Timer from "timer";
import I2C from "pins/i2c";

const addresses = new Uint8Array(128);

function isReserved(address) {
	return (0 == (address & 0x78)) || ((address & 0x78) == 0x78);
}

function scan() {
	for (let address = 0x00; address <= 0x7F; address++) {
		if (isReserved(address))
			continue;

		const i2c = new I2C({address, timeout: 50});
		let found = undefined !== i2c.read(1);
		i2c.close();

		if (found && !addresses[address])
			trace(`Found 0x${address.toString(16).padStart(2, "0")}\n`);
		else if (!found && addresses[address])
			trace(`Lost 0x${address.toString(16).padStart(2, "0")}\n`);

		addresses[address] = found;
	}

	Timer.set(scan);
}

scan();
