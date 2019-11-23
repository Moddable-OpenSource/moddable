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
import Net from "net";
import Timer from "timer";

class Ping @ "xs_ping_destructor" {
	constructor(data, callback) {
		// Sample data: { host: "www.moddable.com", id: 0, interval: 5000 }
		this.client = callback;
		this.id = data.id;
		this.interval = data.interval || 5000;
		this.icmp_seq = -1;
		this.size = 56;
		this.start(data.host);
	}
	
	configure() @ "xs_ping_configure"
		
	start(host) {
		Net.resolve(host, (host, address) => {
			if (address) {
				this.address = address;
				this.configure(this.address, this.size);
				trace(`PING ${host} (${address})\n`);
				this.ping();
				this.timer = Timer.repeat(() => {
					if (!this.reply)
						this.client(2);
					this.ping();
				}, this.interval);
			}
			else this.failed(`Cannot resolve ${host}`);
		});
	}
	ping() {
		this.icmp_seq++;
		this.reply = false;
		this._ping();
	}
	failed(message) {
		this.client(-1, message);
		this.close();
	}
	close() {
		if (this.timer) {
			Timer.clear(this.timer);
			delete this.timer;
		}
		this._close();
	}
	callback(message, value) {
		let icmp_seq = this.icmp_seq;
		let address = this.address;
		this.reply = true;

		if (-1 == message)
			this.failed("Invalid response for icmp_seq " + icmp_seq);
		else
			this.client(message, value, {address, icmp_seq});
	}

	_close() @ "xs_ping_close"
	_ping() @ "xs_ping"
};
Object.freeze(Ping.prototype);

export default Ping;

