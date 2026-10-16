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
	ESP8266 OTA update writes the application slot that is not running (the "nextota" flash partition).
	complete() validates the image and requests the slot switch, which happens at the next restart (embedded:system restart()).

	The image CRC-32 covers every byte that precedes it. In append mode, the CRC accumulates as the image is written,
	holding back the last 4 bytes written, which are the CRC itself when the image ends there. Otherwise, complete()
	calculates the CRC by reading the image back.
*/

import {CRC32} from "crc";

const kBlockLength = 4096;
const kReadLength = 1024;
const kV2ImageMagic = 0xEA;
const kV2ImageSegment = 4;
const kImageMagic = 0xE9;
const kFlashSize4MBc1 = 6;		// esptool flash size code: two 1 MB application slots

const token = Object.freeze({});

function checkPartition(partition) { return native("xs_update_esp_partition").call(this, partition); }
function commit() { return native("xs_update_esp_commit").call(this); }

class Update {
	#partition;
	#byteLength;
	#append;
	#position = 0;		// append mode: offset of next write
	#erased = 0;		// append mode: count of erased blocks
	#blocks;			// write mode: bitmap of erased blocks
	#crc32;				// append mode: CRC-32 of bytes written, excluding #tail
	#crc;
	#tail;				// append mode: last 4 bytes written
	#tailLength = 0;

	constructor(options, key) {
		if (token !== key)
			throw new TypeError;

		const partition = options.partition;
		if (!partition)
			throw new Error("no partition");
		checkPartition(partition);

		let append = true;
		const mode = options.mode;
		if (undefined !== mode) {
			if ("w" === mode)
				append = false;
			else if ("a" !== mode)
				throw new RangeError("invalid mode");
		}

		const size = partition.status().size;
		const byteLength = options.byteLength;
		if ((undefined !== byteLength) && !((byteLength >= 0) && (byteLength <= size)))
			throw new RangeError("invalid byteLength");

		this.#partition = partition;
		this.#byteLength = size;
		this.#append = append;
		if (append) {
			this.#crc32 = newCRC32();
			this.#tail = new Uint8Array(4);
		}
		else
			this.#blocks = new Uint8Array(Math.ceil(size / kBlockLength / 8));
	}
	close() {
		this.#partition = undefined;
		this.#blocks = undefined;
		this.#crc32?.close();
		this.#crc32 = undefined;
	}
	complete() {
		const partition = this.#partition;
		if (!partition)
			throw new Error("already complete");

		const end = findCRC(partition);
		let valid = undefined !== end;
		if (valid) {
			if (this.#append && ((end + 4) === this.#position)) {
				const tail = this.#tail;
				valid = esp8266CRC(this.#crc) === ((tail[0] | (tail[1] << 8) | (tail[2] << 16) | (tail[3] << 24)) >>> 0);
			}
			else
				valid = validate(partition, end);
		}
		this.close();

		if (undefined === end)
			throw new Error("invalid image");
		if (!valid)
			throw new Error("invalid image CRC");

		commit();
	}
	write(buffer, byteOffset) {
		const partition = this.#partition;
		if (!partition)
			throw new Error("already complete");

		if (this.#append) {
			if (undefined != byteOffset)
				throw new Error("offset not allowed");
			byteOffset = this.#position;
		}
		else if (!Number.isInteger(byteOffset) || (byteOffset < 0))
			throw new RangeError("invalid offset");

		const byteLength = buffer.byteLength;
		const end = byteOffset + byteLength;
		if (end > this.#byteLength)
			throw new RangeError("out of range");

		if (this.#append) {
			const last = Math.ceil(end / kBlockLength);
			if (last > this.#erased) {
				partition.eraseBlock(this.#erased, last);
				this.#erased = last;
			}
		}
		else {
			const blocks = this.#blocks;
			for (let block = Math.floor(byteOffset / kBlockLength), last = Math.ceil(end / kBlockLength); block < last; block++) {
				const mask = 1 << (block & 7);
				if (!(blocks[block >> 3] & mask)) {
					partition.eraseBlock(block);
					blocks[block >> 3] |= mask;
				}
			}
		}

		partition.write(buffer, byteOffset);

		if (this.#append) {
			this.#accumulate(ArrayBuffer.isView(buffer) ? new Uint8Array(buffer.buffer, buffer.byteOffset, byteLength) : new Uint8Array(buffer));
			this.#position = end;
		}
	}
	#accumulate(bytes) {
		const tail = this.#tail, byteLength = bytes.length, tailLength = this.#tailLength;

		if ((tailLength + byteLength) <= 4) {
			tail.set(bytes, tailLength);
			this.#tailLength = tailLength + byteLength;
			return;
		}

		// release the bytes that are no longer among the last 4 written: first from the tail, then from bytes
		const release = tailLength + byteLength - 4;
		const fromTail = Math.min(tailLength, release);
		if (fromTail) {
			this.#crc = this.#crc32.checksum(tail.subarray(0, fromTail));
			tail.copyWithin(0, fromTail, tailLength);
		}
		const fromBytes = release - fromTail;
		if (fromBytes)
			this.#crc = this.#crc32.checksum(bytes.subarray(0, fromBytes));
		tail.set(bytes.subarray(fromBytes), tailLength - fromTail);
		this.#tailLength = 4;
	}

	get format() {
		return "buffer";
	}
	set format(value) {
		if (value != "buffer")
			throw new RangeError("invalid");
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

function newCRC32() {
	return new CRC32(0x04C11DB7, 0xFFFFFFFF, true, true, 0xFFFFFFFF);
}

function esp8266CRC(crc) {		// esptool esp8266_crc32
	return (crc & 0x80000000) ? ((crc ^ 0xFFFFFFFF) >>> 0) : (crc + 1);
}

/*
	Image written by esptool elf2image --version=2:
		header (magic 0xEA, 4, flash mode, flash size & frequency, entry)
			only an image built for the two slot layout (flash size 4MB-c1) may be installed
		irom0 segment (address, byteLength, bytes)
		header (magic 0xE9, segment count, flash mode, flash size & frequency, entry)
		segments loaded to RAM (address, byteLength, bytes)
		checksum of RAM segment bytes, as the last byte of a 16 byte block
		CRC-32 of everything before it

	The CRC covers the checksum byte, so a matching CRC implies a matching checksum.
*/
function findCRC(partition) {		// returns offset of CRC, undefined if image is invalid
	const size = partition.status().size;
	const bytes = new Uint8Array(16);
	const view = new DataView(bytes.buffer);

	if (!read(0, 16) || (kV2ImageMagic !== bytes[0]) || (kV2ImageSegment !== bytes[1]) || (kFlashSize4MBc1 !== (bytes[3] >> 4)))
		return;
	let position = 16 + view.getUint32(12, true);

	if (!read(position, 8) || (kImageMagic !== bytes[0]))
		return;
	let segments = bytes[1];
	position += 8;

	while (segments--) {
		if (!read(position, 8))
			return;
		position += 8 + view.getUint32(4, true);
	}

	position = (position | 15) + 1;
	if ((position + 4) > size)
		return;
	return position;

	function read(position, byteLength) {
		if ((position + byteLength) > size)
			return false;
		partition.read(bytes.subarray(0, byteLength), position);
		return true;
	}
}

function validate(partition, end) {
	const bytes = new Uint8Array(kReadLength);
	const crc32 = newCRC32();
	let crc;

	for (let offset = 0; offset < end; offset += kReadLength) {
		const use = Math.min(end - offset, kReadLength);
		partition.read(bytes.subarray(0, use), offset);
		crc = crc32.checksum(bytes.subarray(0, use));
	}
	crc32.close();

	partition.read(bytes.subarray(0, 4), end);
	return esp8266CRC(crc) === ((bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24)) >>> 0);
}

export default Object.freeze({
	open(options) {
		return new Update(options, token);
	},
}, true);
