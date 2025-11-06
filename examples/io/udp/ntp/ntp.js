/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

import Timer from "timer"

class NTP {
	#socket;
	#servers;
	#index;
	#retry;
	#dns;
	#address;
	#callback;
	#timer;
	
	constructor(options) {
		const {servers, socket, dns, target} = options;
		if (!servers?.length || !socket || !dns)
			throw new Error("invalid");
		
		if (undefined !== target)
			this.target = target;
		
		this.#servers = servers;
		this.#dns = new dns.io(dns);
		this.#socket = new socket.io({
			...socket,
			onReadable: count => this.#onReadable(count)
		});
	}
	close() {
		this.#socket?.close();
		this.#socket = undefined;

		this.#dns?.close();
		this.#dns = undefined;

		Timer.clear(this.#timer);
		this.#timer = undefined;
	}
	getTime(callback) {
		if (undefined !== this.#callback)
			throw new Error("in use");

		this.#callback = callback;
		this.#index = 0;
		this.#retry = 0;
		this.#address = undefined;

		this.#next();
	}
	#next() {
		if (this.#address) {
			if (this.#retry++ < 5) {
				const packet = new Uint8Array(48);
				packet[0] = (4 << 3) | (3 << 0);		// version 4, mode 3 (client)
				this.#socket.write(packet, this.#address, 123);
				
				Timer.clear(this.#timer);
				this.#timer = Timer.set(() => this.#next(), 2000);
				return;
			}

			this.#address = undefined;
			this.#retry = 0;
		}

		Timer.clear(this.#timer);
		this.#timer = undefined;

		if (this.#index === this.#servers.length) {
			const callback = this.#callback;
			this.#callback = undefined;
			return callback(new Error);
		}

		this.#dns.resolve({
			host: this.#servers[this.#index++],
			onResolved: (host, address) => {
				this.#address = address;
				this.#next();
			},
			onError: () => this.#next()
		});
	}
	#onReadable(count) {
		const packet = new DataView(this.#socket.read());
		while (--count)
			this.#socket?.read();

		Timer.clear(this.#timer);
		this.#timer = undefined;

		const callback = this.#callback;
		this.#callback = undefined;

		callback?.(null, (packet.getUint32(40) - 2208988800) * 1000);
	}
}

export default NTP;
