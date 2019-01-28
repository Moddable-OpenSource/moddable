/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
	Commodetto PNG reader
		

	To do:


*/

import Bitmap from "Bitmap";

export default class PNG @ "xs_PNG_destructor" {
	constructor(buffer) @ "xs_PNG_constructor"
	read() @ "xs_PNG_read"

	get width() @ "xs_PNG_get_width"
	get height() @ "xs_PNG_get_height"

	get channels() @ "xs_PNG_get_channels"
	get depth() @ "xs_PNG_get_depth"

//	get palette()

	static decompress(data, pixelFormat, alpha) {
		let BufferOut = require.weak("commodetto/BufferOut");
		let Converter = require.weak("commodetto/Convert");
		let png = new PNG(data);
		let width = png.width, height = png.height, channels = png.channels, depth = png.depth, palette = png.palette ? new Uint8Array(png.palette) : undefined;

		if ((8 == depth) && (((3 == channels) || (4 == channels)) || ((1 == channels) && palette))) {		// RGB (24) or RGBA (32) or indexed (8)
			let image = new BufferOut({width, height, pixelFormat});
			image.begin(0, 0, width, height);
			let scanOut = new ArrayBuffer((width * Bitmap.depth(pixelFormat)) >> 3);
			let convert = new Converter((3 == channels) ? Bitmap.RGB24 : Bitmap.RGBA32, pixelFormat);

			let mask, scanAlpha;
			if (alpha && ((4 == channels) || palette)) {
				mask = new BufferOut({width: width, height: height, pixelFormat: alpha});		//@@ only works with Gray16 buffer
				mask.begin(0, 0, width, height);
				scanAlpha = new Uint8Array(scanOut);
			}

			for (let i = 0; i < height; i++) {
				let scanLine = png.read();

				if (palette) {
//@@ need a converter!
					for (let j = 0; j < width; j++) {
						let offset = scanLine[j] * 4;
						let r = palette[offset    ];
						let g = palette[offset + 1];
						let b = palette[offset + 2];
						scanOut[j] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
					}
				}
				else
					convert.process(scanLine.buffer, scanOut);
				image.send(scanOut);

				if (mask) {
//@ converters!
					if (palette) {
						for (let j = 0; j < width; j += 2) {
							let a0 = palette[(scanLine[j] * 4) + 3];
							let a1 = palette[(scanLine[j + 1] * 4) + 3];
							scanAlpha[j >> 1] = ~((a0 & 0xf0) | (a1 >> 4));
						}
					}
					else {
						for (let j = 0, offset = 3; j < (width >> 1); j++, offset += (channels << 1)) {
							let a0 = scanLine[offset];
							let a1 = scanLine[offset + channels];
							scanAlpha[j] = ~((a0 & 0xf0) | (a1 >> 4));
						}
					}
					mask.send(scanAlpha.buffer, 0, width >> 1);
				}
			}

			image.end();
			image = image.bitmap;
			if (mask) {
				mask.end();
				image.mask = mask.bitmap;
			}

			return image;
		}
		else
		if ((8 == depth) && (1 == channels) && !palette) {		// G (8)
			let image = new BufferOut({width: width, height: height, pixelFormat: Bitmap.Gray16});

			image.begin(0, 0, width, height);
			let scanAlpha = new Uint8Array(width >> 1);

			for (let i = 0; i < height; i++) {
				let scanLine = png.read();
				for (let j = 0; j < width; j += 2) {
					let a0 = scanLine[j];
					let a1 = scanLine[j + 1];
					scanAlpha[j >> 1] = (a0 & 0xf0) | (a1 >> 4);
				}
				image.send(scanAlpha.buffer, 0, width >> 1);
			}

			image.end();
			return image.bitmap;
		}
		else if ((1 == depth) && (1 == channels) && !palette) {		// BW (1)
			let image = new BufferOut({width: width, height: height, pixelFormat: Bitmap.Monochrome});

			image.begin(0, 0, width, height);
			let scanAlpha = new Uint8Array(width >> 3);

			for (let i = 0; i < height; i++) {
				let scanLine = png.read();
				for (let j = 0; j < scanLine.byteLength; j++)
					scanAlpha[j] = ~scanLine[j];
				image.send(scanAlpha.buffer);
			}

			image.end();
			return image.bitmap;
		}

		throw new Error("PNG.decompress doesn't support PNG variant")
	}
}
