/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
	constructor(data, callback) {
		// Sample data: { host: "www.moddable.com", id: 0, interval: 5000 }
		super({kind: "RAW", protocol: 1});
		this.client = callback;
		this.id = data.id;
		this.interval = data.interval || 5000;
		this.icmp_seq = -1;
		this.start(data.host);
	}
	start(host) {
		Net.resolve(host, (host, address) => {
			if (address) {
				this.address = address;
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
		let address = this.address;
		let packet = this.packet = new ArrayBuffer(56);		// 8 for icmp header + 48 for icmp payload
		let values = new Uint8Array(packet);

		// ICMP header
		values[0] = 8;										// type 8 (echo request)
		values[1] = 0;										// code 0
		values[2] = values[3] = 0;							// will be the checksum
		values[4] = (this.id & 0xFF00) >> 8;
		values[5] =	this.id & 0x00FF;
		values[6] = (this.icmp_seq & 0xFF00) >> 8;
		values[7] =	this.icmp_seq & 0x00FF;
		for (let i=8, val=0x08; val<0x38; val+=1, i++) {	// packet data
			values[i] = val;
		}
		let cs = Ping.checksum(values);
		values[2] = (cs & 0xFF00) >> 8;
		values[3] = cs & 0x00FF;

		this.reply = false;
		this.write(address, packet);
	}
	failed(message) {
		this.client(-1, message);
		this.close();
	}
	close() {
		Timer.clear(this.timer);
		delete this.timer;
		super.close();
	}
	callback(message, value, address) {
		if (2 !== message) return;
		this.reply = true;
		if (value != 76) this.failed("Unexpected packet length");

		// Ignore IP header
		this.read(null, 20);

		// ICMP header
		let buf = this.read(ArrayBuffer, 56);
		let values = new Uint8Array(buf);
		if (values[0] != 0 || values[1] != 0) this.failed("Response is not an echo reply");
		let checksum = (values[2] << 8) + values[3];
		let identifier = (values[4] << 8) + values[5];
		let icmp_seq = (values[6] << 8) + values[7];
		let isValid = Ping.validate_checksum(identifier, icmp_seq, checksum);
		if (isValid) {
			this.client(1, value: value-20, {address, icmp_seq});
		} else {
			this.failed("Invalid checksum for icmp_seq "+icmp_seq);
		}
	}

	static checksum(values) {
		let sum = 0;
		for (let i=0; i<values.length; i+=2) {
			sum += (values[i] << 8) + values[i+1];
		}
		let carry = sum & 0xFFFF0000;
		sum += (carry >> 16);
		return ~sum & 0xFFFF;
	}
	static validate_checksum(identifier, seqNumber, checksum) {
		let sum = 191232;									// sum of packet bytes (0x08, 0x09...0x37)
		sum += identifier + seqNumber;
		let carry = sum & 0xFFFF0000;
		sum += (carry >> 16);
		return checksum == (~sum & 0xFFFF);
	}
};
Object.freeze(Ping.prototype);

export default Ping;
