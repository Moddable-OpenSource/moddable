import {Socket} from "socket";
import Net from "net";

// modeled on ESP8266mDNS.cpp

const MDNS_TYPE_AAAA = 0x001C;
const MDNS_TYPE_A = 0x0001;
const MDNS_TYPE_PTR = 0x000C;
const MDNS_TYPE_SRV = 0x0021;
const MDNS_TYPE_TXT = 0x0010;

const MDNS_CLASS_IN = 0x0001;
const MDNS_NAME_REF = 0xC000;

const MDNS_IP = "224.0.0.251";
const MDNS_PORT = 5353;

const LOCAL = "local";

class MDNS extends Socket {
	constructor(dictionary) {
		super({kind: "UDP", port: MDNS_PORT, multicast: MDNS_IP});

		this.hostName = dictionary.hostName;
		this.instanceName = dictionary.instanceName ? dictionary.instanceName : this.hostName;
		this.services = [];
	}
	add(service) {
		this.services.push(service);
		this.write(MDNS_IP, MDNS_PORT, this.reply(0x0F, service, false));		// remove before adding to clear stale state in clients
		this.write(MDNS_IP, MDNS_PORT, this.reply(0x0F, service));
	}
	update(service) {
		const index = this.services.indexOf(service);
		if (index < 0) throw new Error("service not found");

		this.write(MDNS_IP, MDNS_PORT, this.reply(0x04, service));
	}
	remove(service) {
		const index = this.services.indexOf(service);
		if (index < 0) throw new Error("service not found");

		this.services.splice(index, 1);
		this.write(MDNS_IP, MDNS_PORT, this.reply(0x0F, service, true));
	}

	callback(message, value, address, port) {
		const header = new Uint8Array(this.read(ArrayBuffer, 12));
		if (header[2] & 0x80)
			return;

		let protocol, local, service;

		let hostName = this.read(String, this.read(Number));
		if ("_" == hostName[0]) {
			service = hostName.slice(1);
			hostName = undefined;
		}

		if (hostName && (hostName !== this.hostName) && (this.instanceName !== hostName))
			return;

		if (undefined === service) {
			service = this.read(String, this.read(Number));
			if ("_" == service[0])
				service = service.slice(1);
			else
			if (LOCAL === service) {
				if (0 !== this.read(Number))
					return;
				protocol = "";
				local = "";
			}
			else
				service = undefined;
		}

		if (undefined === protocol) {
			protocol = this.read(String, this.read(Number));
			if ((4 === protocol.length) && ("_" == protocol[0]))
				protocol = protocol.slice(1);

			if (("services" === service) && ("_dns-sd" == protocol)) {
				this.services.forEach(service => this.write(address, port, this.reply(0x0F, service)));
				return;
			}
		}

		if (undefined === local) {
			local = this.read(String, this.read(Number));
			let temp = this.read(Number);
			if ((5 === local.length) && (LOCAL === local) && (0 == temp))
				;
			else
				return;
		}

		if (!service || !protocol)
			return;

		service = this.services.find(item => (item.name === service) && (item.protocol === protocol))
		if (!service)
			return;

		// respond
		let numQuestions = Math.min((header[4] << 8) | header[5], 4);
		let mask = 0;
		while (numQuestions--) {
			let cType = this.read(Number) << 8;
			cType |= this.read(Number);
			if (MDNS_NAME_REF & cType) {
				cType = this.read(Number) << 8;
				cType |= this.read(Number);
			}
			let cClass = this.read(Number) << 8;
			cClass |= this.read(Number);
			if (MDNS_CLASS_IN & cClass) {
				if (MDNS_TYPE_A === cType) mask |= 1;
				else if (MDNS_TYPE_SRV === cType) mask |= 3;
				else if (MDNS_TYPE_TXT === cType) mask |= 4;
				else if (MDNS_TYPE_PTR === cType) mask |= 0x0F;
			}

			if (numQuestions > 0) {
				let temp = this.read(Number) << 8;
				temp |= this.read(Number);
				if (0xC00C != temp)
					numQuestions = 0;
			}
		}

		this.write(address, port, this.reply(mask, service));
	}
	reply(mask, service, bye = false) {
		bye = bye ? 0 : 0xFF;
		const answerCount = (mask & 1) + ((mask & 2) >> 1) + ((mask & 4) >> 2) + ((mask & 8) >> 3);
		const packet = [Uint8Array.of(0, 0, 0x84, 0, 0, 0, 0, answerCount, 0, 0, 0, 0)];

		if (8 & mask) {		// PTR
			packet.push("_" + service.name, "_" + service.protocol, LOCAL, 0);

			// the type, class, ttl and rdata length
			let ptrDataLen = this.instanceName.length + (service.name.length + 1) + (service.protocol.length + 1) + 5 + 5; // 5 is four label sizes and the terminator
			packet.push(Uint8Array.of(0, 0x0C, 0, 1, 0, 0, 0x11 & bye, 0x94 & bye, 0, ptrDataLen));

			// the RData (ie. "My IOT device._http._tcp.local")
			packet.push(this.instanceName, "_" + service.name, "_" + service.protocol, LOCAL, 0);
		}

		if (4 & mask) {	// TXT
			packet.push(this.instanceName, "_" + service.name, "_" + service.protocol, LOCAL, 0);

			//Send the type, class, ttl and rdata length
			let length = 0;
			if (service.txt) {
				for (let property in service.txt)
					length += property.length + 1 + service.txt[property].toString().length + 1;
			}
			packet.push(Uint8Array.of(0, 0x10, 0x80, 1, 0, 0, 0x11 & bye, 0x94 & bye, 0, length));

			//Send the RData
			if (length) {
				for (let property in service.txt)
					packet.push(property + "=" + service.txt[property]);
			}
		}

		if (2 & mask) {	// SRV
			packet.push(this.instanceName, "_" + service.name, "_" + service.protocol, LOCAL, 0);

			let srvDataSize = this.hostName.length + 5 + 3; // 3 is 2 lable size bytes and the terminator
			srvDataSize += 6; // Size of Priority, weight and port
			packet.push(Uint8Array.of(0, 0x21, 0x80, 1, 0, 0, 0 & bye, 0x78 & bye, 0, srvDataSize));
			packet.push(Uint8Array.of(0, 0, 0, 0, service.port >> 8, service.port & 255));

			//RData (ie. "esp8266.local")
			packet.push(this.hostName, LOCAL, 0);
		}

		if (1 & mask) {	// A
			packet.push(this.hostName, LOCAL, 0);
			packet.push(Uint8Array.of(0, 1, 0x80, 1, 0, 0, 0 & bye, 0x78 & bye, 0, 4));
			// RData
			let ip = Net.get("IP").split(".");
			packet.push(Uint8Array.from(ip.map(value => parseInt(value))));
		}

		let position = packet.reduce((position, value) => {
			const type = typeof value;
			if ("number" === type)
				return position + 1;
			if ("string" === type)
				return position + 1 + value.length;
			return position + value.byteLength;
		}, 0);

		let buffer = new Uint8Array(position);
		position = 0;
		packet.forEach(value => {
			const type = typeof value;
			if ("number" === type) {
				buffer[position] = value;
				position += 1;
			}
			else if ("string" === type) {
				buffer[position++] = value.length;
				for (let i = 0; i < value.length; i++)
					buffer[position++] = value.charCodeAt(i);
			}
			else {
				buffer.set(value, position);
				position += value.byteLength;
			}
		});

		return buffer.buffer;
	}
}
Object.freeze(MDNS.prototype);

export default MDNS;
