/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
	Commodetto RLE4 in Memory
		
		implements PixelOut to write a run-length encoded bitmap to ArrayBuffer.
			Optimized for images with large runs of solid or transparent pixels (fonts, image masks)

		series of runs. each run starts with 4-bits that includes type and pixel count. three types of runs:

			SKIP: 0nnn (2 to 9 transparent pixels)
			SOLID: 10nn (2 to 5 unblended black pixels)
			QUOTE: 11nn + (1 to 4 pixels)

		no restriction on width or height

		scan lines are packed (e.g. scan line may begin mid-byte)
*/

import PixelsOut from "commodetto/PixelsOut";
import Bitmap from "commodetto/Bitmap";

export default class RLE4Out extends PixelsOut {
	constructor(dictionary) {
		super(dictionary);

		if (Bitmap.Gray16 !== this.pixelFormat)
			throw new Error("requires Gray16 pixels");
	}
	begin(x, y, width, height) {
		// buffer for expanded scanline
		width = (width + 1) & ~1;
		this.scan = new Uint8Array(width);

		// buffer to hold worst-case encoded image
		this.output = new Uint8Array(width * (height + (height >> 2)));
		this.output.position = 0;
		this.output.firstNybble = true;
	}
	send(pixels, offsetIn = 0, count = pixels.byteLength - offsetIn) {
		let width = this.width;
		let halfWidth = (width + 1) >> 1;

		let lines = Math.floor((count * 2) / width);
		let scan = this.scan;
		const solid = 0;
		const skip = 15;

		pixels = new Uint8Array(pixels);
		for (let line = 0, offset = offsetIn; line < lines; line++, offset += halfWidth) {
			// expand incoming packed 4-bit gray pixels into bytes (if width is odd, expands one extra (unused) pixel)
			for (let i = 0, pos = offset; i < width; i += 2, pos += 1) {
				scan[i + 0] = pixels[pos] >> 4;
				scan[i + 1] = pixels[pos] & 0x0F;
			}

			// encode one run
			for (let remain = width, pos = 0; remain > 0; ) {
				if (remain >= 2) {
					// check for skip run
					if ((skip === scan[pos]) && (skip === scan[pos + 1])) {
						let count = 2;
						while (((remain - count) > 0) && (count < 9) && (skip === scan[pos + count]))
							count += 1;
						this.writeNybble(count - 2);
						remain -= count;
						pos += count;
						continue;
					}

					// check for solid run
					if ((solid === scan[pos]) && (solid === scan[pos + 1])) {
						let count = 2;
						while (((remain - count) > 0) && (count < 5) && (solid === scan[pos + count]))
							count += 1;
						this.writeNybble(0x08 | (count - 2));
						remain -= count;
						pos += count;
						continue;
					}
				}

				// quote
				let quoteLen = 1;
				while (quoteLen < 4) {
					if (remain < (quoteLen + 2)) {
						quoteLen = Math.min(remain, 4);
						break;
					}
					if ((scan[pos + quoteLen] === scan[pos + quoteLen + 1]) &&
						((solid === scan[pos + quoteLen]) || (skip === scan[pos + quoteLen])))
						break;
					quoteLen += 1;
				}

				this.writeNybble(0x0C | (quoteLen - 1));
				for (let i = 0; i < quoteLen; i++)
					this.writeNybble(scan[pos + i]);
				remain -= quoteLen;
				pos += quoteLen;
			}
		}
	}
	end() {
		delete this.scan;

		if (!this.output.firstNybble)
			this.output.position += 1;

		this.bits = new Bitmap(this.width, this.height, Bitmap.RLE | Bitmap.Gray16,
							this.output.slice(0, this.output.position).buffer, 0);

		delete this.output;

	}
	pixelsToBytes(count) {
		return (count * this.depth + 7) >> 3;
	}
	get bitmap() {
		return this.bits;
	}

	writeNybble(value) {
		let output = this.output;

		if ((value < 0) || (value > 15))
			throw new Error("invalid nybble");

		if (output.firstNybble) {
			output[output.position] = value;

			output.firstNybble = false;
		}
		else {
			output[output.position] |= value << 4;

			output.position += 1;
			output.firstNybble = true;
		}
	}
}

Object.freeze(RLE4Out.prototype);
