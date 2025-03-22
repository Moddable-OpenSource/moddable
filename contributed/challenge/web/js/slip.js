/*
 * Copyright (c) 2016-2024 Moddable Tech, Inc.
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

class SLIP {
	static unescape(payload) {
		let state = -1;	// -1 syncing, 0 receiving, 1 pending escape
		let packet, start;
		payload = new Uint8Array(payload);
		for (let position = 0, length = payload.byteLength; position < length; position++) {
			if (-1 === state) {
				position = payload.indexOf(0xc0, position);
				if (-1 === position)
					break;
				if (-1 === payload.indexOf(0xc0, position + 1)) {
					state = 0;
					start = position;
					break;
				}
			}

			let byte = payload[position]
			if (1 === state) {
				if (0xdd === byte)
					byte = 0xdb;
				else if (0xdc === byte)
					byte = 0xc0;
				else {
					state = -1;
					continue;
				}
				state = 0;
				packet[packet.position++] = byte;
				continue;
			}

			if (0xc0 === byte) {
				if ((state >= 0) && packet.position) {
					position++;
					packet.buffer.resize(packet.position);
					const result = {packet: packet.buffer};
					if (position < length)
						result.remainder = payload.buffer.slice(position);
					return result;
				}
				state = 0;
				packet = new Uint8Array(new ArrayBuffer(length - position, {maxByteLength: length - position}));
				packet.position = 0;
				start = position;
				continue;
			}

			if (0xdb === byte) {
				state = 1;
				continue;
			}
			
			if (-1 === state)
				continue;

			packet[packet.position++] = byte;
		}
		
		if (-1 === state)
			return {};

		return {remainder: payload.buffer.slice(start)};
	}

	static escape(payload) {
		payload = new Uint8Array(payload);
		const length = payload.length;
		let escapes = 0;
		for (let i = 0; i < length; i++) {
			const byte = payload[i];
			if ((0xdb === byte) || (0xc0 === byte))
				escapes += 1;
		}
		const data = new Uint8Array(2 + length + escapes);
		data[0] = 0xc0;
		for (let i = 0, o = 1; i < length; i++) {
			const byte = payload[i];
			if (0xdb === byte) {
				data[o++] = 0xdb;
				data[o++] = 0xdd;
			}
			else
			if (0xc0 === byte) {
				data[o++] = 0xdb;
				data[o++] = 0xdc;
			}
			else
				data[o++] = byte;
		}
		data[data.length - 1] = 0xc0;
		return data.buffer;
	}
}

export default SLIP;
