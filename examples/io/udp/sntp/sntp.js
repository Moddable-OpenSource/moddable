/*
 * Copyright (c) 2019  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

 import UDP from "builtin/socket/udp";

class SNTP extends UDP {
	#onTime;
	#onError;
	#timer;
	constructor(dictionary) {
		super({});

		this.#onTime = dictionary.onTime || this.onTime;
		if (!this.#onTime)
			throw new Error("onTime required");

		this.#onError = dictionary.onError || this.onError;

		System.resolve(dictionary.host, (name, address) => {
			if (!address) {
				if (this.#onError)
					this.#onError();
				return;
			}

			request.call(this, address);
			this.#timer = System.setInterval(() => request.call(this, address), 5 * 1000);
		});
	}
	close() {
		if (this.#timer)
			System.clearInterval(this.#timer);
		super.close();
	}
	onReadable(count) {
		let packet;

		while (count--)
			packet = new DataView(this.read());

		System.clearInterval(this.#timer);
		this.#timer = undefined;

		this.#onTime((packet.getUint32(40) - 2208988800) * 1000);		// convert from NTP to Unix Epoch time in milliseconds
	}
}

function request(address)
{
	const packet = new Uint8Array(48);
	packet[0] = (4 << 3) | (3 << 0);		// version 4, mode 3 (client)
	this.write(address, 123, packet);
}

export default SNTP;
