/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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
import {Socket} from "socket";
import Net from "net";
import Timer from "timer";

class Ping extends Socket {
	#callback;
	#timer;
	#identifier;
	#icmp_seq = -1;
	#reply;
	#address;

		// Sample options: { host: "www.moddable.com", id: 0, interval: 5000 }
	constructor(options, callback) {
		if (!callback) throw new Error;

		super({kind: "RAW", protocol: 1});

		this.#callback = callback;
		this.#identifier = options.id;

		Net.resolve(options.host, (host, address) => {
			if (!this.#callback)
				return;		// closed

			if (address) {
				this.#address = address;
				this.ping();
				this.#timer = Timer.repeat(() => {
					if (!this.#reply)
						this.#callback(Ping.timeout);
					if (this.#callback)
						this.ping();
				}, options.interval ?? 5000);
			}
			else
				this.failed(`Cannot resolve ${host}`);
		});
	}
	ping() {
		this.#icmp_seq++;
		const values = new Uint8Array(56);					// 8 for icmp header + 48 for icmp payload

		// ICMP header
		values[0] = 8;										// type 8 (echo request)
//		values[1] = 0;										// code 0
//		values[2] = values[3] = 0;						// will be the checksum
		values[4] = this.#identifier >> 8;
		values[5] = this.#identifier;
		values[6] = this.#icmp_seq >> 8;
		values[7] = this.#icmp_seq;
		for (let i=8, val=0x08; val<0x38; val+=1, i++) {	// packet data
			values[i] = val;
		}
		const checksum = Ping.checksum(values);
		values[2] = checksum >> 8;
		values[3] = checksum;

		this.#reply = false;
		this.write(this.#address, values.buffer);
	}
	failed(message) {
		this.#callback(Ping.error, message);
		this.close();
	}
	close() {
		Timer.clear(this.#timer);
		this.#timer = undefined;
		this.#callback = undefined;

		super.close();
	}
	callback(message, value, address) {
		if (2 !== message) return;
		if (76 !== value)
			return;		// not expected size

		// Ignore IP header
		this.read(null, 20);

		// ICMP header
		const values = new Uint8Array(this.read(ArrayBuffer, 56));
		if (values[0] || values[1])
			return;		// not an echo reply
		const checksum = (values[2] << 8) + values[3];
		const identifier = (values[4] << 8) + values[5];
		if (identifier !== this.#identifier)
			return;		// not for this instance
		this.#reply = true;
		const icmp_seq = (values[6] << 8) + values[7];
		if (!Ping.validate_checksum(identifier, icmp_seq, checksum))
			this.failed("Invalid checksum for icmp_seq "+icmp_seq);

		this.#callback(Ping.response, value - 20, {address, icmp_seq, identifier});
	}

	static checksum(values) {
		let sum = 0;
		for (let i=0; i<values.length; i+=2) {
			sum += (values[i] << 8) + values[i+1];
		}
		sum += (sum >> 16);
		return ~sum & 0xFFFF;
	}
	static validate_checksum(identifier, seqNumber, checksum) {
		let sum = 191232;									// sum of packet bytes (0x08, 0x09...0x37)
		sum += identifier + seqNumber;
		sum += (sum >> 16);
		return checksum === (~sum & 0xFFFF);
	}
	
	static error = -1;
	static response = 1;
	static timeout = 2;
}

export default Ping;
