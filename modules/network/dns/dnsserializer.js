/*
 * Copyright (c) 2018-2026 Moddable Tech, Inc.
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

import DNS from "dns";

class Serializer {
	sections = [[], [], [], [], []];

	constructor(dictionary) {
		let opcode = 0;
		if (undefined !== dictionary.opcode)
			opcode = dictionary.opcode << 3;
		if ((undefined !== dictionary.query) && !dictionary.query)
			opcode |= 0x80;
		if (dictionary.authoritative)
			opcode |= 0x04;
		if (dictionary.recursionDesired)
			opcode |= 0x01;
		this.opcode = opcode;
		if (dictionary.id)
			this.id = dictionary.id;
	}
	splitName(name) {
		if (!name)
			return [];

		const parts = name.split(".");
		if (name.endsWith("."))
			parts.pop();
		return parts;
	}
	add(section, name, type, clss, ttl, data) {
		if (!data)
			;
		else if (data instanceof ArrayBuffer)
			data = new Uint8Array(data);
		else if (undefined !== data.byteLength)
			data = new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
		else {
			let d;

			switch (type) {
				case DNS.RR.A:
					data = Uint8Array.from(data.split(".").map(value => parseInt(value)));
					break;

				case DNS.RR.NSEC:
					if (data.bitmaps instanceof ArrayBuffer)
						data = {next: this.splitName(data.next), bitmaps: new Uint8Array(data.bitmaps)};
					else
						data = {next: this.splitName(data.next), bitmaps: new Uint8Array(data.bitmaps.buffer, data.bitmaps.byteOffset, data.bitmaps.byteLength)};
					break;

				case DNS.RR.PTR:
					data = {name: this.splitName(data)};
					break;

				case DNS.RR.SRV:
					data = {
						priority: data.priority,
						weight: data.weight,
						port: data.port,
						target: this.splitName(data.target)
					};
					break;

				case DNS.RR.TXT:
					d = 0;
					if (Array.isArray(data)) {
						for (let property in data) {
							const value = data[property];
							if (undefined === value) continue;
							d += property.length + 1 + ArrayBuffer.fromString(value.toString()).byteLength + 1;
						}
					}
					else {
						for (const [property, value] in data) {
							if (undefined === value) continue;
							d += property.length + 1 + ArrayBuffer.fromString(value.toString()).byteLength + 1;
						}
					}
					if (d) {
						let offset = 0;
						const binary = new Uint8Array(d);
						if (Array.isArray(data)) {
							for (let property in data) {
								let value = data[property];
								if (undefined === value) continue;
								value = ArrayBuffer.fromString(property + "=" + value.toString());
								binary[offset] = value.byteLength;
								offset += 1;
								binary.set(new Uint8Array(value), offset);
								offset += value.byteLength;
							}
						}
						else {
							for (let [property, value] in data) {
								if (undefined === value) continue;
								value = ArrayBuffer.fromString(property + "=" + value.toString());
								binary[offset] = value.byteLength;
								offset += 1;
								binary.set(new Uint8Array(value), offset);
								offset += value.byteLength;
							}
						}
						data = binary;
					}
					else
						data = new Uint8Array(1);
					break;
			}
		}

		this.append(section, {
			name: this.splitName(name),
			type,
			clss,
			ttl,
			data
		});
	}
	append(section, record) {
		this.sections[section].push(record);
	}
	writeU8(value) {
		const state = this.state;
		if (state.result)
			state.result[state.position] = value;
		state.position += 1;
	}
	writeU16(value) {
		const state = this.state;
		if (state.result) {
			state.result[state.position] = value >> 8;
			state.result[state.position + 1] = value;
		}
		state.position += 2;
	}
	writeU32(value) {
		const state = this.state;
		if (state.result) {
			state.result[state.position] = value >> 24;
			state.result[state.position + 1] = value >> 16;
			state.result[state.position + 2] = value >> 8;
			state.result[state.position + 3] = value;
		}
		state.position += 4;
	}
	writeBytes(bytes) {
		const state = this.state;
		if (state.result)
			state.result.set(bytes, state.position);
		state.position += bytes.byteLength;
	}
	writeName(parts) {
		const state = this.state;
		for (let i = 0; i < parts.length; i++) {
			const suffix = parts.slice(i).join(".");
			const target = state.nameOffsets.get(suffix);
			if (undefined !== target) {
				this.writeU16(0xC000 | target);
				return;
			}

			if (state.position < 0x4000)
				state.nameOffsets.set(suffix, state.position);

			const label = ArrayBuffer.fromString(parts[i]);
			this.writeU8(label.byteLength);
			this.writeBytes(new Uint8Array(label));
		}

		this.writeU8(0);
	}
	writeRData(type, data) {
		if (!data)
			return;

		if (data instanceof Uint8Array)
			this.writeBytes(data);
		else if (DNS.RR.PTR === type)
			this.writeName(data.name);
		else if (DNS.RR.NSEC === type) {
			this.writeName(data.next);
			this.writeBytes(data.bitmaps);
		}
		else if (DNS.RR.SRV === type) {
			this.writeU16(data.priority);
			this.writeU16(data.weight);
			this.writeU16(data.port);
			this.writeName(data.target);
		}
		else
			this.writeBytes(data);
	}
	writeRecord(record) {
		this.writeName(record.name);
		this.writeU16(record.type);
		this.writeU16(record.clss);

		if (undefined === record.ttl)
			return;

		this.writeU32(record.ttl);

		const state = this.state;
		const position = state.position;
		this.writeU16(0);
		let length = state.position;
		this.writeRData(record.type, record.data);

		if (state.result) {
			length = state.position - length;
			state.result[position] = length >> 8;
			state.result[position + 1] = length;
		}
	}
	writeSections() {
		for (let i = 0, sections = this.sections, length = sections.length; i < length; i++)
			sections[i].forEach(record => this.writeRecord(record));
	}
	build() {
		const sections = this.sections;
		this.state = {position: 12, nameOffsets: new Map};
		this.writeSections();

		const result = new Uint8Array(this.state.position);
		const id = this.id ?? 0;
		result.set(Uint8Array.of(id >> 8, id, this.opcode, 0, 0, sections[0].length, 0, sections[1].length, 0, sections[2].length, 0, sections[3].length), 0);		// header

		this.state = {position: 12, nameOffsets: new Map, result};
		this.writeSections();
		delete this.state;
		return result.buffer;
	}
}

export default Serializer;
