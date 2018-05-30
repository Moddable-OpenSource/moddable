/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

class REPL @ "xs_repl_destructor" {
	static receive() @ "xs_repl_receive"
	static write() @ "xs_repl_write"
	static eval() @ "xs_repl_eval"
	static isDebug() @ "xs_repl_isDebug"

	constructor() {
		while (undefined !== REPL.receive())	// flush pending input
			;

		REPL.write("\nModdable REPL (version 0.0.1)\n\n");
		if (REPL.isDebug())
			REPL.write("** Warning: debug build detected. Release build recommended. **\n");
		this.prompt();
		Timer.repeat(this.poll.bind(this), 10);
	}
	prompt() {
		this.incoming = "";
		REPL.write("> ");
	}
	poll() {
		do {
			let c = REPL.receive();
			if (undefined === c)
				return;

			if (c < 32) {
				if ((10 === c) || (13 === c))
					break;

				if (3 === c) {
					REPL.write("^C\n");
					this.prompt();
				}

				if (8 === c) {
					if (this.incoming.length) {
						this.incoming = this.incoming.slice(0, -1);
						REPL.write(String.fromCharCode(8), " ", String.fromCharCode(8));
					}
				}

//				REPL.write(`(ASCII ${c})\n`);

				continue;
			}

			if (127 === c) {
				if (this.incoming.length) {
					this.incoming = this.incoming.slice(0, -1);
					REPL.write(String.fromCharCode(8), " ", String.fromCharCode(8));
				}
				continue;
			}

			c = String.fromCharCode(c);
			REPL.write(c);
			this.incoming += c;
		} while (true);

		REPL.write("\n");
		if (this.incoming) {
			try {
				let result = REPL.eval(this.incoming);
				if (undefined === result)
					REPL.write("undefined\n");
				else if (null === result)
					REPL.write("null\n");
				else
					REPL.write(result.toString(), "\n");
			}
			catch (e) {
				REPL.write(e.toString(), "\n");
			}
		}
		this.prompt();
	}
}
Object.freeze(REPL.prototype);

global.console = {
	log: function (str) {
		REPL.write(str, "\n");
	}
}
Object.freeze(global.console);

export default REPL;
