/*
* Copyright (c) 2021  Moddable Tech, Inc.
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

import Poco from "commodetto/Poco";
import Bitmap from "commodetto/Bitmap";

function drawBitmapWithKeyColor(bits, x, y, color) @ "xs_poco_drawGIF";
Poco.prototype.drawBitmapWithKeyColor = function(bits, x, y, color, flip) {
	if (Bitmap.Monochrome === bits.pixelFormat)
		return this.drawMonochrome(bits, this.makeColor(0, 0, 0), this.makeColor(255, 255, 255), x, y);

	return drawBitmapWithKeyColor.call(this, bits, x, y, color, flip);
}
	
