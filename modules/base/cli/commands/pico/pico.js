/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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

class Pico {
	static restart(how) @ "xs_pico_restart";
}

CLI.install(function(command, parts) {
	switch (command) {
		case "pico":
			command = parts.shift().toLowerCase();
			switch (command) {
				case "restart":
					if (!parts.length) {
						this.line("Restarting Pico");
						Pico.restart();
					}
					else {
						this.line("Restarting Pico in programming mode");
						Pico.restart(true);
					}
					break;

				default:
					this.line("unrecognized command: " + command);
					break;
			}
			break;

		case "help":
			this.line("pico restart - restart device");
			this.line("pico restart prog - restart in programming mode");
			break;

		default:
			return false;
	}

	return true;
});
