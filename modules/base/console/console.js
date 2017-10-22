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
import Timer from "timer";

class Console @ "xs_console_destructor" {
	receive() @ "xs_console_receive"
	write() @ "xs_console_write"

	constructor() {
		CLI.execute.call(this, "welcome");
		this.resume();
	}

	resume() {
		this.timer = Timer.repeat(() => {
			while (true) {
				let c = this.receive();
				if (undefined === c)
					return;

				this.write(String.fromCharCode(c));
				if ((10 === c) || (13 === c)){
					CLI.execute.call(this, this.incoming);
					this.incoming = "";
					this.prompt();
				} else if (127 == c) {
					if (this.incoming.length) {
						this.incoming = this.incoming.slice(0, -1);
						this.write(String.fromCharCode(8), " ", String.fromCharCode(8));
					}
				}
				else
					this.incoming += String.fromCharCode(c);
			};
		}, 10);

		while (undefined !== this.receive())	// flush pending input
			;

		this.prompt();

		this.incoming = "";
	}

	suspend() {
		Timer.clear(this.timer);
		delete this.timer;
	}

	prompt() {
		if (undefined !== this.timer)
			CLI.execute.call(this, "prompt");
	}

	line(...items) {
		this.write(...items, "\r\n");
	}
}
Object.freeze(Console.prototype);

export default Console;
