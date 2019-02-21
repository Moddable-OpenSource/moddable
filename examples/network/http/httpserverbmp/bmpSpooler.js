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

import Poco from "commodetto/Poco";
import Bitmap from "commodetto/Bitmap";
import PixelsOut from "commodetto/PixelsOut";

class BMPSpooler extends PixelsOut {
	constructor(width, height, draw) {
		super({width, height, pixelFormat: Bitmap.RGB565LE});
		this.draw = draw;
		this.render = new Poco(this);
		this.buffers = [this.header()];
		this.byteLength = this.buffers[0].byteLength + this.pixelsToBytes(width * height);
		this.rowBytes = this.pixelsToBytes(width);
		this.y = 0;
	}

	get(byteLength) {
		let result = new Uint8Array(new ArrayBuffer(byteLength));
		let writeOffset = 0;
		while (byteLength) {
			if (!this.buffers.length) {
				this.more(Math.max(1, ((byteLength + this.rowBytes - 1) / this.rowBytes) | 0));
				if (!this.buffers.length) {
					if (!writeOffset)
						return;
					return result.slice(0, writeOffset).buffer;
				}
			}

			let src = this.buffers[0];
			let use = src.byteLength;

			if (use <= byteLength)
				this.buffers.shift();
			else {
				use = byteLength;
				this.buffers[0] = src.slice(use, src.byteLength);
			}
			result.set(new Uint8Array(src, 0, use), writeOffset);

			writeOffset += use;
			byteLength -= use;
		}

		return result.buffer;
	}

	more(lines) {
		this.render.begin(0, this.y, this.width, lines);
			this.draw(this.render, this.width, this.height);
		this.render.end();
		this.y += lines;
	}

	send(pixels, offset = 0, count = pixels.byteLength - offset) {
		this.buffers.push(new Uint8Array(pixels).slice(offset, offset + count).buffer);
	}

	header() {
		const buffer = new ArrayBuffer(0x46);
		const header = new DataView(buffer);
		const littleEndian = 1;
		let offset = 0;

		if (this.width & 1)
			throw new Error("bad width");

		header.setUint8(offset++, 'B'.charCodeAt());						// imageFileType
		header.setUint8(offset++, 'M'.charCodeAt());

		header.setUint32(offset, (this.rowBytes * this.height) + 0x46, littleEndian); offset += 4;	// fileSize
		header.setUint16(offset, 0), offset += 2;							// reserved1
		header.setUint16(offset, 0), offset += 2;							// reserved2
		header.setUint32(offset, 0x46, littleEndian); offset += 4;			// imageDataOffset

		header.setUint32(offset, 0x38, littleEndian); offset += 4;			// biSize
		header.setUint32(offset, this.width, littleEndian); offset += 4;	// biWidth
		header.setInt32(offset, -this.height, littleEndian); offset += 4;	// biHeight (negative, because we write top-to-bottom)
		header.setUint16(offset, 1, littleEndian), offset += 2;				// biPlanes
		header.setUint16(offset, 16, littleEndian), offset += 2;			// biBitCount
		header.setUint32(offset, 3, littleEndian), offset += 4;				// biCompression (3 == 565 pixels (see mask below), 0 == 555 pixels)
		header.setUint32(offset, (this.rowBytes * this.height) + 2, littleEndian); offset += 4;	// biSizeImage

		header.setUint32(offset, 0x0b12, littleEndian); offset += 4;		// biXPelsPerMeter
		header.setUint32(offset, 0x0b12, littleEndian); offset += 4;		// biYPelsPerMeter
		header.setUint32(offset, 0); offset += 4;							// biClrUsed
		header.setUint32(offset, 0); offset += 4;							// biClrImportant

		[0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x1F,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
				.forEach(value => header.setUint8(offset++, value));	// masks for 565 pixels

		return buffer;
	}
}
Object.freeze(BMPSpooler.prototype);

export default BMPSpooler;
