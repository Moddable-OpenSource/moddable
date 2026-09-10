/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

/*
	u-blox UBX binary protocol (minimal)

	Frame: B5 62 | class | id | length(u2 LE) | payload | CK_A | CK_B

	Enough for GNSS location sensors that stream UBX-NAV-PVT.

	GPS protocol contract (embedded:sensor/Location/GPS-UART, GPS-I2C):
	  new Protocol([onParsed])
	  Protocol.push(buffer | byte) -> parsed message or undefined
	  Protocol.location(parsed)    -> Location-oriented update or undefined
	  Protocol.identification
	  Protocol.setup()             -> CFG-MSG enabling NAV-PVT on DDC
*/

const SYNC1 = 0xB5;
const SYNC2 = 0x62;
const CLASS_NAV = 0x01;
const ID_NAV_PVT = 0x07;
const CLASS_CFG = 0x06;
const ID_CFG_MSG = 0x01;
const MAX_PAYLOAD = 128;		// NAV-PVT is 92 bytes

const identification = Object.freeze({
	model: "u-blox UBX GNSS",
	classification: "Location",
	description: "u-blox receiver (UBX-NAV-PVT)"
});

/**
 * UBX Fletcher-8 checksum over bytes [offset, offset+length].
 * @returns {[number, number]} CK_A, CK_B
 */
function checksum(bytes, offset, length) {
	let ckA = 0, ckB = 0;
	const end = offset + length;
	for (let i = offset; i < end; i++) {
		ckA = (ckA + bytes[i]) & 0xFF;
		ckB = (ckB + ckA) & 0xFF;
	}
	return [ckA, ckB];
}

/**
 * Build a complete UBX frame.
 * @param {number} msgClass
 * @param {number} msgId
 * @param {ArrayBuffer|ArrayBufferView} [payload]
 * @returns {ArrayBuffer}
 */
function format(msgClass, msgId, payload) {
	let payBytes;
	if (!payload)
		payBytes = new Uint8Array(0);
	else if (payload instanceof ArrayBuffer)
		payBytes = new Uint8Array(payload);
	else
		payBytes = new Uint8Array(payload.buffer, payload.byteOffset, payload.byteLength);

	const out = new Uint8Array(8 + payBytes.length);
	out[0] = SYNC1;
	out[1] = SYNC2;
	out[2] = msgClass;
	out[3] = msgId;
	out[4] = payBytes.length & 0xFF;
	out[5] = (payBytes.length >> 8) & 0xFF;
	if (payBytes.length)
		out.set(payBytes, 6);
	const [ckA, ckB] = checksum(out, 2, 4 + payBytes.length);
	out[6 + payBytes.length] = ckA;
	out[7 + payBytes.length] = ckB;
	return out.buffer;
}

/**
 * Parse a complete UBX frame. Returns null if invalid.
 * NAV-PVT yields location-oriented fields (SI units where practical).
 */
function parse(frame) {
	const bytes = frame instanceof ArrayBuffer
		? new Uint8Array(frame)
		: new Uint8Array(frame.buffer, frame.byteOffset, frame.byteLength);
	if (bytes.length < 8 || bytes[0] !== SYNC1 || bytes[1] !== SYNC2)
		return null;
	const payloadLen = bytes[4] | (bytes[5] << 8);
	if (bytes.length !== 8 + payloadLen)
		return null;
	const [ckA, ckB] = checksum(bytes, 2, 4 + payloadLen);
	if (ckA !== bytes[6 + payloadLen] || ckB !== bytes[7 + payloadLen])
		return null;

	const msgClass = bytes[2];
	const msgId = bytes[3];
	if (msgClass === CLASS_NAV && msgId === ID_NAV_PVT && payloadLen >= 92)
		return parseNAVPVT(new DataView(bytes.buffer, bytes.byteOffset + 6, payloadLen));

	return {
		type: "UBX",
		class: msgClass,
		id: msgId,
		payloadLength: payloadLen
	};
}

/**
 * UBX-NAV-PVT payload → structured object.
 * lat/lon deg, altitude/accuracy m, speed m/s, course deg.
 */
function parseNAVPVT(view) {
	const fixType = view.getUint8(20);
	const flags = view.getUint8(21);
	const gnssFixOK = !!(flags & 0x01);
	const fix = gnssFixOK && fixType >= 2;

	return {
		type: "PVT",
		class: CLASS_NAV,
		id: ID_NAV_PVT,
		fixType,
		latitude: view.getInt32(28, true) * 1e-7,
		longitude: view.getInt32(24, true) * 1e-7,
		altitude: view.getInt32(32, true) / 1000,			// height above ellipsoid, m
		accuracy: view.getUint32(40, true) / 1000,			// hAcc, m
		altitudeAccuracy: view.getUint32(44, true) / 1000,	// vAcc, m
		speed: view.getInt32(60, true) / 1000,				// gSpeed, m/s
		course: view.getInt32(64, true) * 1e-5,				// headMot, deg
		pdop: view.getUint16(76, true) * 0.01,
		satellites: view.getUint8(23),
		fix,
		status: fix ? "A" : "V",
		quality: fix ? (fixType >= 3 ? 1 : fixType) : 0
	};
}

function wrap360(value) {
	value %= 360;
	if (value < 0)
		value += 360;
	return value;
}

/**
 * Location-oriented update from a parsed message.
 * Valid NAV-PVT fix → position; NAV-PVT without fix → { fix: false } so GPS can clear.
 */
function location(parsed) {
	if (!parsed || parsed.type !== "PVT")
		return;
	if (!parsed.fix)
		return { fix: false };

	const update = {
		latitude: parsed.latitude,
		longitude: parsed.longitude,
		fix: true
	};
	if (parsed.altitude != null)
		update.altitude = parsed.altitude;
	if (parsed.accuracy != null)
		update.accuracy = parsed.accuracy;
	if (parsed.altitudeAccuracy != null)
		update.altitudeAccuracy = parsed.altitudeAccuracy;
	if (parsed.speed != null)
		update.speed = parsed.speed;
	if (parsed.course != null)
		update.heading = wrap360(parsed.course);
	return update;
}

/**
 * Command frames to write after open: enable NAV-PVT on DDC and UART1.
 * Payload: msgClass, msgId, rateDDC, rateUART1, rateUART2, rateUSB, rateSPI, reserved.
 */
function setup() {
	return [
		format(CLASS_CFG, ID_CFG_MSG, Uint8Array.of(CLASS_NAV, ID_NAV_PVT, 1, 1, 0, 0, 0, 0))
	];
}

/**
 * Incremental frame reassembler. Feed a buffer or a single byte.
 * @returns {object|undefined} the last parsed message when a complete frame is ready
 */
class Parser {
	#buf = new Uint8Array(6 + MAX_PAYLOAD + 2);
	#length = 0;
	#payloadLen = 0;
	#state = 0;
	#onParsed;

	constructor(onParsed) {
		this.#onParsed = onParsed;
	}

	#consume(byte) {
		switch (this.#state) {
			case 0:
				if (byte === SYNC1) {
					this.#buf[0] = byte;
					this.#length = 1;
					this.#state = 1;
				}
				return;
			case 1:
				if (byte === SYNC2) {
					this.#buf[1] = byte;
					this.#length = 2;
					this.#state = 2;
				}
				else {
					this.#state = byte === SYNC1 ? 1 : 0;
					this.#length = byte === SYNC1 ? 1 : 0;
					if (byte === SYNC1)
						this.#buf[0] = SYNC1;
				}
				return;
			case 2:
				this.#buf[this.#length++] = byte;
				if (this.#length < 6)
					return;
				this.#payloadLen = this.#buf[4] | (this.#buf[5] << 8);
				if (this.#payloadLen > MAX_PAYLOAD) {
					this.#state = 0;
					this.#length = 0;
					return;
				}
				this.#state = 3;
				return;
			case 3: {
				this.#buf[this.#length++] = byte;
				const need = 8 + this.#payloadLen;
				if (this.#length < need)
					return;
				const frame = this.#buf.buffer.slice(this.#buf.byteOffset, this.#buf.byteOffset + need);
				this.#state = 0;
				this.#length = 0;
				return parse(frame) ?? undefined;
			}
		}
	}

	push(data) {
		let last;
		if (typeof data === "number") {
			last = this.#consume(data);
			if (last)
				this.#onParsed?.(last);
			return last;
		}
		const bytes = data instanceof ArrayBuffer
			? new Uint8Array(data)
			: new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
		for (let i = 0; i < bytes.length; i++) {
			const parsed = this.#consume(bytes[i]);
			if (parsed) {
				last = parsed;
				this.#onParsed?.(parsed);
			}
		}
		return last;
	}
}

Object.assign(Parser, {
	SYNC1,
	SYNC2,
	CLASS_NAV,
	ID_NAV_PVT,
	CLASS_CFG,
	ID_CFG_MSG,
	checksum,
	format,
	parse,
	parseNAVPVT,
	location,
	identification,
	setup
});

export default Parser;

