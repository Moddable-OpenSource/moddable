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

import {Socket, Listener} from "socket";
import CLI from "cli";

class Connection extends Socket {
	constructor(dictionary) {
		super(dictionary);
		this.initialize = Symbol();
		CLI.distribute.call(this, this.initialize);
		this.incoming = "";
		this.write(255, 251, 1, 255, 251, 3, 255, 252, 34);		// character, not line, mode
	}
	line(...items) {
		this.write(...items, "\r\n");
	}
	callback(message, count) {
		switch (message) {
			case 2:		// data ready
				while (count--) {
					let byte = this.read(Number);
					if (255 === byte) {
						this.command = true;
						continue;
					}
					if (this.command) {
						if (true === this.command) {
							if (244 === byte) {		// Control C (Interrupt Process)
								if (this.suspended) {
									if (true !== this.suspended)
										this.suspended();
									delete this.suspended;
								}
								this.incoming = "";
								continue;
							}
							this.command = byte;
						}
						else {
							// process 3-byte command
							if (253 === this.command) {		// DO request
								this.write(255, 252, byte, 13, 10);		// WONT response followed by CRLF
								CLI.execute.call(this, "prompt");	// control C now over
							}
							delete this.command;
						}
						continue;
					}
					if (this.suspended)
						continue;
					if (13 === byte)
						continue;
					if ((8 === byte) || (127 === byte)) {		//@@ is this ever used @@
						if (this.incoming.length)
							this.incoming.length -= 1;
						continue;
					}
					if (10 === byte) {
						CLI.execute.call(this, this.incoming);
						this.incoming = "";
						if (!this.suspended)
							CLI.execute.call(this, "prompt");
						continue;
					}

					this.incoming += String.fromCharCode(byte);
				}
				break;

			default:
				if (message < 0)
					trace("connection lost\n");
				break;
		}
	}
	// placeholders
	suspend(cancel) {
		this.suspended = cancel || true;
	}
	resume() {
		if (!this.suspended) return;

		delete this.suspended;
		CLI.execute.call(this, "prompt");
	}
}
Object.freeze(Connection.prototype);

class Telnet extends Listener {
	constructor(dictionary = {}) {
		super(Object.assign({port: 23}, dictionary));
	}
	callback() {
		const socket = new Connection({listener: this});
		CLI.execute.call(socket, "welcome");
		CLI.execute.call(socket, "prompt");
	}
}
Object.freeze(Telnet.prototype);

export default Telnet;
