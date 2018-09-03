import DNS from "dns";

class Serializer {
	constructor(dictionary) {
		this.sections = [[], [], [], [], []];
		let opcode = 0;
		if (undefined !== dictionary.opcode)
			opcode = dictionary.opcode << 3;
		if ((undefined !== dictionary.query) && !dictionary.query)
			opcode |= 0x80;
		if (dictionary.authoritative)
			opcode |= 0x04;
		this.opcode = opcode;
		if (dictionary.id)
			this.id = dictionary.id;
	}
	add(section, name, type, clss, ttl, data) {
		const record = [];

		if (!data)
			;
		else if (data instanceof ArrayBuffer)
			data = new Uint8Array(data);
		else {
			let d;

			switch (type) {
				case DNS.RR.A:
					data = Uint8Array.from(data.split(".").map(value => parseInt(value)));
					break;

				case DNS.RR.NSEC:
					let next = data.next.split(".").map(item => ArrayBuffer.fromString(item));
					d = new Uint8Array(next.reduce((value, item) => value + item.byteLength + 1, data.bitmaps.byteLength));
					let offset = next.reduce((offset, item) => {
						d[offset] = item.byteLength;
						d.set(new Uint8Array(item), offset + 1);
						return offset + 1 + item.byteLength;
					}, 0);
					d.set(new Uint8Array(data.bitmaps), offset);
					data = d;
					break;

				case DNS.RR.PTR:
					data = data.split(".").map(item => ArrayBuffer.fromString(item));
					d = new Uint8Array(data.reduce((value, item) => value + item.byteLength + 1, 1));
					data.reduce((offset, item) => {
						d[offset] = item.byteLength;
						d.set(new Uint8Array(item), offset + 1);
						return offset + 1 + item.byteLength;
					}, 0);
					data = d;
					break;

				case DNS.RR.SRV:
					let target = data.target.split(".").map(item => ArrayBuffer.fromString(item));
					d = new Uint8Array(target.reduce((value, item) => value + item.byteLength + 1, 6 + 1));
					d[0] = data.priority >> 8;
					d[1] = data.priority;
					d[2] = data.weight >> 8;
					d[3] = data.weight;
					d[4] = data.port >> 8;
					d[5] = data.port;
					target.reduce((offset, item) => {
						d[offset] = item.byteLength;
						d.set(new Uint8Array(item), offset + 1);
						return offset + 1 + item.byteLength;
					}, 6);
					data = d;
					break;

				case DNS.RR.TXT:
					d = 0;
					for (let property in data) {
						const value = data[property];
						if (undefined === value) continue;
						d += property.length + 1 + ArrayBuffer.fromString(value.toString()).byteLength + 1;
					}
					if (d) {
						let offset = 0;
						let binary = new Uint8Array(d);
						for (let property in data) {
							let value = data[property];
							if (undefined === value) continue;
							value = ArrayBuffer.fromString(property + "=" + value.toString());
							binary[offset] = value.byteLength;
							offset += 1;
							binary.set(new Uint8Array(value), offset);
							offset += value.byteLength;
						}
						data = binary;
					}
					else
						data = new Uint8Array(1);
					break;
			}
		}

		if (name)
			name.split(".").forEach(item => record.push(item));
		record.push(0);
		if (undefined !== ttl) {
			record.push(Uint8Array.of(0, type,
									  clss >> 8, clss,
									  (ttl >> 24) & 0xff, (ttl >> 16) & 0xff, (ttl >> 8) & 0xff, ttl & 0xff,
									  data ? data.byteLength >> 8 : 0, data ? data.byteLength : 0));
			if (data)
				record.push(data)
		}
		else
			record.push(Uint8Array.of(0, type,
									  clss >> 8, clss));
		this.append(section, record);
	}
	append(section, parts) {
		const byteLength = parts.reduce((byteLength, value) => {
			const type = typeof value;
			if ("number" === type)
				return byteLength + 1;
			if ("string" === type)
				return byteLength + 1 + ArrayBuffer.fromString(value).byteLength;
			return byteLength + value.byteLength;
		}, 0);

		let fragment = new Uint8Array(byteLength);
		let position = 0;
		parts.forEach(value => {
			const type = typeof value;
			if ("number" === type) {
				fragment[position] = value;
				position += 1;
			}
			else if ("string" === type) {
				let s = ArrayBuffer.fromString(value);
				fragment[position++] = s.byteLength;
				fragment.set(new Uint8Array(s), position);
				position += s.byteLength;
			}
			else {
				fragment.set(value, position);
				position += value.byteLength;
			}
		});

/*
		// check for duplicate answer
		for (let i = 0; i < this.fragments.length; i++) {
			const k = this.fragments[i];
			if (k.byteLength !== byteLength)
				continue;
			for (let j = 0; j < byteLength; j++) {
				if (fragment[j] !== k[j])
					continue fragments;
			}
			return;
		}
*/
		this.sections[section].push(fragment);
	}
	build() {
		const sections = this.sections;
		let byteLength = 0;
		for (let i = 0; i < 4; i++)
			 byteLength += sections[i].reduce((byteLength, value) => byteLength + value.byteLength, 0);
		let position = 12;
		let result = new Uint8Array(byteLength + position);
		result.set(Uint8Array.of(this.id >> 8, this.id & 255, this.opcode, 0, 0, sections[0].length, 0, sections[1].length, 0, sections[2].length, 0, sections[3].length), 0);		// header
		for (let i = 0; i < 4; i++)
			sections[i].forEach(fragment => {
				result.set(fragment, position);
				position += fragment.byteLength;
			});
		return result.buffer;
	}
}
Serializer.prototype.id = 0;
Object.freeze(Serializer.prototype);

export default Serializer;
