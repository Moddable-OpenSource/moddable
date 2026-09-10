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
	NMEA 0183

	Parses common GNSS sentences (GGA, RMC, GSA, GSV, VTG, ZDA, GLL)
	produced by modules such as ATGM336H, NEO-M8N, NEO-6M, and any
	NMEA-0183 compatible receiver (GPS, BDS, GLONASS, Galileo, multi-constellation).

	Talker IDs supported: GP, GN, GL, GA, GB, BD, QZ, etc. (first two chars after $).
	Sentences beginning $P are proprietary and are not decoded as standard types.

	GPS protocol contract (embedded:sensor/Location/GPS-UART, GPS-I2C):
	  new Protocol([onParsed])
	  Protocol.push(buffer | byte) -> parsed sentence or undefined
	  Protocol.location(parsed)    -> Location-oriented update or undefined
	  Protocol.identification
*/

const Talker = Object.freeze({
	GP: "GPS",
	GN: "GNSS",		// multi-constellation (common on dual-mode modules)
	GL: "GLONASS",
	GA: "Galileo",
	GB: "BeiDou",
	BD: "BeiDou",
	QZ: "QZSS",
	P:  "Proprietary"
});

const KNOTS_TO_MS = 0.514444;
const HDOP_TO_METERS = 5;
const MAX_SENTENCE = 256;

const identification = Object.freeze({
	model: "NMEA-0183 GNSS",
	classification: "Location",
	description: "Generic NMEA-0183 GNSS receiver"
});

function hexDigit(c) {
	if (c >= 48 && c <= 57)
		return c - 48;
	if (c >= 65 && c <= 70)
		return c - 55;
	if (c >= 97 && c <= 102)
		return c - 87;
	return -1;
}

function toBytes(sentence) {
	if (typeof sentence !== "string") {
		return sentence instanceof ArrayBuffer
			? new Uint8Array(sentence)
			: new Uint8Array(sentence.buffer, sentence.byteOffset, sentence.byteLength);
	}
	sentence = sentence.trim();
	const length = sentence.length;
	const bytes = new Uint8Array(length);
	for (let i = 0; i < length; i++) {
		const c = sentence.charCodeAt(i);
		if (c > 127)
			return;
		bytes[i] = c;
	}
	return bytes;
}

/**
 * Validate and strip the NMEA checksum.
 * Operates on the sentence bytes (`$...*hh`).
 * @returns {Uint8Array|null} payload without `$` and `*checksum`, or null if invalid
 */
function validateChecksum(sentence) {
	const bytes = toBytes(sentence);
	if (!bytes || bytes.length < 5 || bytes[0] !== 0x24)
		return null;
	let star = -1;
	for (let i = 1; i < bytes.length; i++) {
		if (bytes[i] === 0x2A) {
			star = i;
			break;
		}
	}
	if (star < 0 || star + 2 >= bytes.length)
		return null;
	const d1 = hexDigit(bytes[star + 1]);
	const d2 = hexDigit(bytes[star + 2]);
	if (d1 < 0 || d2 < 0)
		return null;
	let checksum = 0;
	for (let i = 1; i < star; i++)
		checksum ^= bytes[i];
	if (checksum !== ((d1 << 4) | d2))
		return null;
	return bytes.subarray(1, star);
}

/**
 * Convert NMEA latitude/longitude (ddmm.mmmm) + hemisphere to decimal degrees.
 * @param {string} value - e.g. "4807.038"
 * @param {string} hemi - "N","S","E","W"
 * @returns {number|null}
 */
function toDecimalDegrees(value, hemi) {
	if (!value || value.length < 3)
		return null;
	const num = parseFloat(value);
	if (Number.isNaN(num))
		return null;
	const deg = Math.floor(num / 100);
	const minutes = num - (deg * 100);
	let decimal = deg + (minutes / 60);
	if (hemi === "S" || hemi === "W")
		decimal = -decimal;
	return decimal;
}

function wrap360(value) {
	value %= 360;
	if (value < 0)
		value += 360;
	return value;
}

/**
 * Parse a single NMEA sentence into a structured object.
 * Returns null on invalid checksum or unsupported/empty sentence.
 */
function parse(sentence) {
	const payload = validateChecksum(sentence);
	if (!payload)
		return null;

	const text = String.fromArrayBuffer(payload.buffer, payload.byteOffset, payload.byteLength);
	const fields = text.split(",");
	if (fields.length < 1)
		return null;

	const id = fields[0];					// e.g. "GNGGA" or "PGRMC"
	if (!id)
		return null;

	let talker, type;
	if (id[0] === "P") {
		talker = "P";
		type = id.slice(1);
	}
	else {
		if (id.length < 3)
			return null;
		talker = id.slice(0, 2);
		type = id.slice(2);
	}

	const result = {
		type,
		talker,
		system: Talker[talker] ?? talker,
		raw: text
	};

	switch (type) {
		case "GGA": {	// Global Positioning System Fix Data
			// $--GGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
			result.time = fields[1] || null;
			result.latitude = toDecimalDegrees(fields[2], fields[3]);
			result.longitude = toDecimalDegrees(fields[4], fields[5]);
			result.quality = fields[6] ? parseInt(fields[6], 10) : 0;
			result.satellites = fields[7] ? parseInt(fields[7], 10) : 0;
			result.hdop = fields[8] ? parseFloat(fields[8]) : null;
			result.altitude = fields[9] ? parseFloat(fields[9]) : null;
			result.geoidSeparation = fields[11] ? parseFloat(fields[11]) : null;
			result.fix = result.quality > 0;
			break;
		}
		case "RMC": {	// Recommended Minimum Specific GNSS Data
			// $--RMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a,a*hh
			result.time = fields[1] || null;
			result.status = fields[2] || null;			// A=active, V=void
			result.latitude = toDecimalDegrees(fields[3], fields[4]);
			result.longitude = toDecimalDegrees(fields[5], fields[6]);
			result.speed = fields[7] ? parseFloat(fields[7]) : null;	// knots
			result.course = fields[8] ? parseFloat(fields[8]) : null;	// degrees
			result.date = fields[9] || null;
			result.mode = fields[12] || null;
			result.fix = result.status === "A";
			break;
		}
		case "GLL": {	// Geographic Position - Latitude/Longitude
			result.latitude = toDecimalDegrees(fields[1], fields[2]);
			result.longitude = toDecimalDegrees(fields[3], fields[4]);
			result.time = fields[5] || null;
			result.status = fields[6] || null;
			result.mode = fields[7] || null;
			result.quality = (result.status === "A") ? 1 : 0;
			result.fix = result.status === "A";
			break;
		}
		case "VTG": {	// Track Made Good and Ground Speed
			result.course = fields[1] ? parseFloat(fields[1]) : null;	// true
			result.speed = fields[5] ? parseFloat(fields[5]) : null;	// knots
			result.mode = fields[9] || null;
			break;
		}
		case "GSA": {	// GNSS DOP and Active Satellites
			result.mode = fields[1] || null;
			result.fixType = fields[2] ? parseInt(fields[2], 10) : 0; // 1=no fix, 2=2D, 3=3D
			result.pdop = fields[15] ? parseFloat(fields[15]) : null;
			result.hdop = fields[16] ? parseFloat(fields[16]) : null;
			result.vdop = fields[17] ? parseFloat(fields[17]) : null;
			result.satellitesUsed = [];
			for (let i = 3; i <= 14; i++) {
				if (fields[i])
					result.satellitesUsed.push(parseInt(fields[i], 10));
			}
			result.satellites = result.satellitesUsed.length;
			result.fix = result.fixType >= 2;
			break;
		}
		case "GSV": {	// GNSS Satellites in View
			result.totalMessages = fields[1] ? parseInt(fields[1], 10) : 0;
			result.messageNumber = fields[2] ? parseInt(fields[2], 10) : 0;
			result.satellitesInView = fields[3] ? parseInt(fields[3], 10) : 0;
			result.satellites = [];
			for (let i = 4; i + 3 < fields.length; i += 4) {
				const prn = fields[i] ? parseInt(fields[i], 10) : null;
				if (prn !== null) {
					result.satellites.push({
						prn,
						elevation: fields[i + 1] ? parseInt(fields[i + 1], 10) : null,
						azimuth: fields[i + 2] ? parseInt(fields[i + 2], 10) : null,
						snr: fields[i + 3] ? parseInt(fields[i + 3], 10) : null
					});
				}
			}
			break;
		}
		case "ZDA": {	// Time & Date
			result.time = fields[1] || null;
			result.day = fields[2] ? parseInt(fields[2], 10) : null;
			result.month = fields[3] ? parseInt(fields[3], 10) : null;
			result.year = fields[4] ? parseInt(fields[4], 10) : null;
			result.date = (result.day && result.month && result.year)
				? `${String(result.day).padStart(2, "0")}${String(result.month).padStart(2, "0")}${String(result.year).slice(-2)}`
				: null;
			break;
		}
		default:
			result.fields = fields.slice(1);
			break;
	}

	return result;
}

function checksum(payload) {
	let c = 0;
	for (let i = 0; i < payload.length; i++)
		c ^= payload.charCodeAt(i);
	return c.toString(16).toUpperCase().padStart(2, "0");
}

/**
 * Build a complete NMEA sentence with checksum.
 * @param {string} payload - e.g. "GNGGA,..." (no $ or *)
 */
function format(payload) {
	return `$${payload}*${checksum(payload)}\r\n`;
}

/**
 * Location-oriented update from a parsed sentence.
 * Speed is m/s; heading is degrees true; accuracy is meters from HDOP.
 * Altitude is GGA field 9 (MSL) plus field 11 (geoid separation) when present;
 * otherwise field 9 alone.
 */
function location(parsed) {
	if (!parsed)
		return;

	const type = parsed.type;
	if (type === "GSV" || type === "ZDA")
		return;

	const update = {};
	let any = false;

	if (type === "GGA") {
		if (parsed.latitude != null && parsed.longitude != null) {
			update.latitude = parsed.latitude;
			update.longitude = parsed.longitude;
			any = true;
		}
		if (parsed.altitude != null) {
			// Field 9 is MSL; field 11 (geoid separation) is often empty — use MSL alone then.
			update.altitude = parsed.geoidSeparation != null
				? parsed.altitude + parsed.geoidSeparation
				: parsed.altitude;
			any = true;
		}
		if (parsed.hdop != null) {
			update.accuracy = parsed.hdop * HDOP_TO_METERS;
			any = true;
		}
		update.fix = !!parsed.fix && (parsed.latitude != null);
		any = true;
	}
	else if (type === "RMC") {
		if (parsed.latitude != null && parsed.longitude != null) {
			update.latitude = parsed.latitude;
			update.longitude = parsed.longitude;
			any = true;
		}
		if (parsed.speed != null) {
			update.speed = parsed.speed * KNOTS_TO_MS;
			any = true;
		}
		if (parsed.course != null) {
			update.heading = wrap360(parsed.course);
			any = true;
		}
		update.fix = parsed.status === "A" && parsed.latitude != null;
		any = true;
	}
	else if (type === "GLL") {
		if (parsed.latitude != null && parsed.longitude != null && parsed.status === "A") {
			update.latitude = parsed.latitude;
			update.longitude = parsed.longitude;
			update.fix = true;
			any = true;
		}
	}
	else if (type === "VTG") {
		if (parsed.speed != null) {
			update.speed = parsed.speed * KNOTS_TO_MS;
			any = true;
		}
		if (parsed.course != null) {
			update.heading = wrap360(parsed.course);
			any = true;
		}
	}
	else if (type === "GSA") {
		if (parsed.hdop != null) {
			update.accuracy = parsed.hdop * HDOP_TO_METERS;
			any = true;
		}
		if (parsed.fixType != null) {
			update.fix = parsed.fixType >= 2;
			any = true;
		}
	}

	return any ? update : undefined;
}

/** Incremental sentence reassembler. Feed a buffer or a single byte. */
class Parser {
	#buf = new Uint8Array(MAX_SENTENCE);
	#length = 0;
	#onParsed;

	constructor(onParsed) {
		this.#onParsed = onParsed;
	}

	#consume(byte) {
		if (byte > 127)
			return;
		if (byte === 0x0A || byte === 0x0D) {
			if (this.#length === 0)
				return;
			const sentence = this.#buf.subarray(0, this.#length);
			this.#length = 0;
			if (sentence[0] !== 0x24)
				return;
			return parse(sentence) ?? undefined;
		}
		if (byte === 0x24) {
			this.#buf[0] = 0x24;
			this.#length = 1;
			return;
		}
		if (this.#length === 0)
			return;
		if (this.#length < this.#buf.length)
			this.#buf[this.#length++] = byte;
		else
			this.#length = 0;
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
	Talker,
	parse,
	validateChecksum,
	toDecimalDegrees,
	checksum,
	format,
	location,
	identification
});

export default Parser;
