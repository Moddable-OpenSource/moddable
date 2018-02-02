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
	Commodetto render
	
		Graphics rendering object
		Delivers rendered pixels to a PixelOut instance
*/

export default class Render {
	/*
		Constructor binds this renderer to a PixelOut instance
		Provides pixels for rendering into
	*/
	constructor(pixelsOut, dictionary) {
		this.pixelsOut = pixelsOut;
	}

	begin(x = 0, y = 0, width = this.pixelsOut.width - x, height = this.pixelsOut.height - y) {
	}
	end() {
	}
	continue(...coodinates) {
		this.pixelsOut.continue();
		this.end();
		this.begin(...coodinates);
	}
}
