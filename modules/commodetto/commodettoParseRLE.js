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
	Commodetto RLE parser
*/

import Bitmap from "commodetto/Bitmap";

/*
export default function parseRLE(buffer) {
	let bytes = new Uint8Array(buffer);

	if ((109 !== bytes[0]) || (100 !== bytes[1]))		// "md"
		throw new Error("invalid commodetto rle");

	if (0 !== bytes[2])
		throw new Error("unsupported version");

	if ((Bitmap.Gray16 | Bitmap.RLE) !== bytes[3])
		throw new Error("unsupported pixel format");

	return new Bitmap((bytes[4] << 8) | bytes[5], (bytes[6] << 8) | bytes[7],
						Bitmap.Gray16 | Bitmap.RLE, buffer, 8);
}

*/

function parse() @ "xs_parseRLE";

export default function (buffer) {
	return parse(buffer, new Bitmap(1, 1, Bitmap.Default, buffer, 0));
}
