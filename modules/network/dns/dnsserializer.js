/*
 * Copyright (c) 2018-2025 Moddable Tech, Inc.
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
	constructor(dictionary) {
		this.sections = [[], [], [], [], []];
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
		if ((parts.length > 1) && ("" === parts[parts.length - 1]))
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
						for (const [property, value] of data) {
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
							for (let [property, value] of data) {
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
	writeU8(state, value) {
		if (state.result)
			state.result[state.position] = value;
		state.position += 1;
	}
	writeU16(state, value) {
		if (state.result) {
			state.result[state.position] = (value >> 8) & 0xFF;
			state.result[state.position + 1] = value & 0xFF;
		}
		state.position += 2;
	}
	writeU32(state, value) {
		if (state.result) {
			state.result[state.position] = (value >> 24) & 0xFF;
			state.result[state.position + 1] = (value >> 16) & 0xFF;
			state.result[state.position + 2] = (value >> 8) & 0xFF;
			state.result[state.position + 3] = value & 0xFF;
		}
		state.position += 4;
	}
	writeBytes(state, bytes) {
		if (state.result)
			state.result.set(bytes, state.position);
		state.position += bytes.byteLength;
	}
	writeName(state, parts) {
		for (let i = 0; i < parts.length; i++) {
			const suffix = parts.slice(i).join(".");
			const target = state.nameOffsets.get(suffix);
			if (undefined !== target) {
				this.writeU16(state, 0xC000 | target);
				return;
			}

			if (state.position < 0x4000)
				state.nameOffsets.set(suffix, state.position);

			const label = ArrayBuffer.fromString(parts[i]);
			this.writeU8(state, label.byteLength);
			this.writeBytes(state, new Uint8Array(label));
		}

		this.writeU8(state, 0);
	}
	writeRData(state, type, data) {
		if (!data)
			return;

		if (data instanceof Uint8Array)
			this.writeBytes(state, data);
		else if (DNS.RR.PTR === type)
			this.writeName(state, data.name);
		else if (DNS.RR.NSEC === type) {
			this.writeName(state, data.next);
			this.writeBytes(state, data.bitmaps);
		}
		else if (DNS.RR.SRV === type) {
			this.writeU16(state, data.priority);
			this.writeU16(state, data.weight);
			this.writeU16(state, data.port);
			this.writeName(state, data.target);
		}
		else
			this.writeBytes(state, data);
	}
	writeRecord(state, record) {
		this.writeName(state, record.name);
		this.writeU16(state, record.type);
		this.writeU16(state, record.clss);

		if (undefined === record.ttl)
			return;

		this.writeU32(state, record.ttl);

		const rdlengthPosition = state.position;
		this.writeU16(state, 0);
		const rdataPosition = state.position;
		this.writeRData(state, record.type, record.data);
		const rdlength = state.position - rdataPosition;

		if (state.result) {
			state.result[rdlengthPosition] = (rdlength >> 8) & 0xFF;
			state.result[rdlengthPosition + 1] = rdlength & 0xFF;
		}
	}
	writeSections(state) {
		for (let i = 0; i < 4; i++) {
			this.sections[i].forEach(record => {
				this.writeRecord(state, record);
			});
		}
	}
	build() {
		const sections = this.sections;
		let state = {position: 12, nameOffsets: new Map};
		this.writeSections(state);

		let result = new Uint8Array(state.position);
		const id = (undefined === this.id) ? 0 : this.id;
		result.set(Uint8Array.of(id >> 8, id & 255, this.opcode, 0, 0, sections[0].length, 0, sections[1].length, 0, sections[2].length, 0, sections[3].length), 0);		// header

		state = {position: 12, nameOffsets: new Map, result};
		this.writeSections(state);
		return result.buffer;
	}
}
Object.freeze(Serializer.prototype);

export default Serializer;
