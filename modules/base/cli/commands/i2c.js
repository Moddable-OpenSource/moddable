/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

import CLI from "cli";
import SMBus from "pins/smbus";
import I2C from "pins/i2c";

CLI.install(function(command, parts) {
	switch (command) {
		case "i2c":
			if (parts.length < 1)
				throw new Error("missing read/write/scan");
			let address = parseInt(parts[1]);
			let mode = parts[0].toLowerCase();
			if ("read" === mode) {
				if (parts.length < 2)
					throw new Error("missing address");
				if (parts.length < 3)
					throw new Error("missing register number");

				let i2c = new SMBus({address});
				let count = (3 == parts.length) ? 1 : parseInt(parts[3]);
				for (let i = 0, register = parseInt(parts[2]); i < count; i++, register++)
					this.line("0x", register.toString(16), ": 0x", i2c.readByte(register).toString(16));
				i2c.close();
			}
			else if ("write" === mode) {
				if (parts.length < 2)
					throw new Error("missing address");
				if (parts.length < 3)
					throw new Error("missing register number");
				if (parts.length < 4)
					throw new Error("missing write value");

				let i2c = new SMBus({address});
				let count = parts.length - 3;
				for (let i = 0, register = parseInt(parts[2]); i < count; i++, register++)
					i2c.writeByte(register, parseInt(parts[3 + i]));
				i2c.close();
			}
			else if ("scan" === mode) {
				for (let address = 1; address < 128; address++) {
					let i2c;
					try {
						i2c = new I2C({address, throw: false});
						i2c.write(0, false);			// set register
						if (i2c.read(1))
							this.line("Found: 0x", address.toString(16));
					}
					catch {
					}
					i2c?.close();
				}
			}
			else
				throw new Error("expected read or write");
			break;

		case "help":
			this.line("i2c read address register [count] - reads register(s)");
			this.line("i2c write address register value [value [value]] - writes value(s) to register(s)");
			this.line("i2c scan - scans for active i2c devices");
			break;

		default:
			return false;
	}

	return true;
});
