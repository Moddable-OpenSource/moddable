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

/*
	sntp
*/

import {Socket} from "socket";
import Timer from "timer";

class SNTP extends Socket {
	constructor(dictionary, callback) {
		super({kind: "UDP"});
		this.address = dictionary.address;
		this.client = callback;

		this.write(this.address, 123, this.packet());

		this.timer = Timer.repeat(timer.bind(this), 5000);
		this.count = 5;		// maximum number of retransmits
	}

	close() {
		Timer.clear(this.timer);
		delete this.timer;

		super.close();
	}

	callback(message, value) {
		if (2 !== message)
			return;

		if (48 !== value)
			this.client(-1, "unexpected SNTP packet length");
		else
		if (0x24 !== this.read(Number))
			this.client(-1, "unexpected SNTP packet first byte");
		else {
			this.read(null, 39);
			this.client(1, this.toNumber(this.read(Number), this.read(Number), this.read(Number), this.read(Number)));
		}
		this.close();
	}

	packet() @ "xs_sntp_packet";
	toNumber() @ "xs_sntp_toNumber";
};

function timer() {
	this.count -= 1;
	if (!this.count) {
		this.client(-1, "failed to contact sntp server");
		this.close();
		return;
	}

	this.client(2);		// retrying
	this.write(this.address, 123, this.packet());
}

Object.freeze(SNTP.prototype);

export default SNTP;
