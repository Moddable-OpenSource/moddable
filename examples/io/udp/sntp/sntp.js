/*
 * Copyright (c) 2019  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

 import UDP from "embedded:io/socket/udp";

class SNTP  {
	#udp;
	#onTime;
	#onError;
	#timer;
	constructor(options) {
		this.#udp = new UDP({
			target: this,
			onReadable: this.#onReadable
		});

		this.#onTime = options.onTime;
		if (!this.#onTime)
			throw new Error("onTime required");

		this.#onError = options.onError;

		System.resolve(options.host, (name, address) => {
			if (!address) {
				this.#onError?.();
				return;
			}

			request.call(this.#udp, address);
			this.#timer = System.setInterval(() => request.call(this.#udp, address), 5 * 1000);
		});
	}
	close() {
		if (this.#timer)
			System.clearInterval(this.#timer);
		super.close();
	}
	#onReadable(count) {
		const target = this.target;
		let packet;

		while (count--)
			packet = new DataView(this.read());

		System.clearInterval(target.#timer);
		target.#timer = undefined;

		target.#onTime((packet.getUint32(40) - 2208988800) * 1000);		// convert from NTP to Unix Epoch time in milliseconds
	}
}

function request(address) {
	const packet = new Uint8Array(48);
	packet[0] = (4 << 3) | (3 << 0);		// version 4, mode 3 (client)
	this.write(address, 123, packet);
}

export default SNTP;
