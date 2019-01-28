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
import Digital from "pins/digital";

CLI.install(function(command, parts) {
	switch (command) {
		case "gpio":
			if (parts.length < 1)
				throw new Error("missing read/write");
			let mode = parts[0].toLowerCase();
			if ("read" === mode) {
				if (parts.length < 2)
					throw new Error("missing pin number");
				this.line(Digital.read(parseInt(parts[1])).toString());
			}
			else if ("write" === mode) {
				if (parts.length < 3)
					throw new Error("missing value");
				Digital.write(parseInt(parts[1]), parseInt(parts[2]));
			}
			else
				throw new Error("expected read or write");
			break;

		case "help":
			this.line("gpio read pin - reads pin");
			this.line("gpio write pin value - writes pin");
			break;

		default:
			return false;
	}

	return true;
});
