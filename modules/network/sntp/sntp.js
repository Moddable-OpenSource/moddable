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

/*
	sntp
*/

import {Socket} from "socket";
import Net from "net";
import Timer from "timer";

class SNTP extends Socket {
	constructor(dictionary, callback) {
		super({kind: "UDP"});
		this.client = callback;
		this.start(dictionary.host);
	}
	start(host) {
		Net.resolve(host, (host, address) => {
			if (address) {
				this.address = address;
				this.write(address, 123, this.packet());
				Timer.schedule(this.timer, 5000, 5000);
			}
			else
				this.failed("cannot resolve " + host);
		});

		if (this.timer)
			Timer.schedule(this.timer, 5000 * 10, 5000 * 10);
		else
			this.timer = Timer.repeat(timer.bind(this), 5000 * 10);
		this.count = 5;		// maximum retransmit attempts
	}
	failed(message) {
		let host = this.client(-1, message);
		if (host)
			this.start(host);
		else
			this.close();
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
			this.failed("unexpected SNTP packet length");
		else
		if (0x24 !== this.read(Number))
			this.failed("unexpected SNTP packet first byte");
		else {
			this.read(null, 39);
			this.client(1, this.toNumber(this.read(Number), this.read(Number), this.read(Number), this.read(Number)));
			this.close();
		}
	}

	packet() @ "xs_sntp_packet";
	toNumber() @ "xs_sntp_toNumber";
};

function timer() {
	this.count -= 1;
	if (!this.count)
		return this.failed("no response from SNTP server");

	this.write(this.address, 123, this.packet());
	this.client(2);		// retrying
}

Object.freeze(SNTP.prototype);

export default SNTP;
