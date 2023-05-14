/*
 * Copyright (c) 2023  Moddable Tech, Inc.
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

// tables based on https://github.com/biril/mp3-parser/blob/master/lib/lib.js
 const bitRates = Object.freeze([
	[],
	[],
	[
		null,
		[0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160],
		[0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160],
		[0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256]
	],
	[
		null,
		[0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320],
		[0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384],
		[0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448]
	],
], true);

const sampleRates = Object.freeze([
	[11025, 12000, 8000],
	[],
	[22050, 24000, 16000],
	[44100, 48000, 32000]
], true);

const sampleLengths = Object.freeze([
	[],
	[576, 1152, 384],
	[576, 1152, 384],
	[1152, 1152, 384],
], true);

export default class @ "xs_mp3_destructor" {
	constructor() @ "xs_mp3";
	close() @ "xs_mp3_close";
	decode(input, output) @ "xs_mp3_decode";
	
	static scan(buffer, start, end, info = {}) {
		let position = start;
		while (position < end) {
			position = buffer.indexOf(0xFF, position);
			if ((position < 0) || ((position + 4) > end))
				return;

			let byte = buffer[++position];
			const mpegVersion = (byte >> 3) & 0x03;
			const layerVersion = (byte >> 1) & 0x03;
			if ((byte < 0xE0) || (1 === mpegVersion) || (0 === layerVersion))
				continue;

			byte = buffer[++position];
			const bitRate = bitRates[mpegVersion][layerVersion][byte >> 4];
			const sampleRate = sampleRates[mpegVersion][(byte >> 2) & 3];
			const padded = (byte >> 1) & 1;
			if ((undefined === bitRate) || (undefined === sampleRate)) {
				position -= 1;
				continue;
			}
			
			let length = sampleLengths[mpegVersion][layerVersion];
			if (!length) {
				position -= 1;
				continue;
			}
			length = Math.idiv(length * (bitRate * 1000 / 8), sampleRate);
			if (padded)
				length += (3 === layerVersion) ? 4 : 1;

			info.position = position - 2;
			info.length = length;
			info.bitRate = bitRate; 
			info.sampleRate = sampleRate; 
			
			return info;
		}
	}
	static BUFFER_GUARD = 8;		// MAD_BUFFER_GUARD == 8
}
