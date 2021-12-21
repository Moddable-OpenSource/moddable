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

class ReadGIF @ "xs_readgif_destructor" {
	constructor(buffer) @ "xs_readgif";
	close() @ "xs_readgif_close";
	first() @ "xs_readgif_first";
	next() @ "xs_readgif_next";

	// bitmap
	get width()  @ "xs_readgif_get_width";
	get height()  @ "xs_readgif_get_height";
	get pixelFormat() @ "xs_readgif_get_pixelFormat"
	get offset() {
		return 0;
	}
	
	// animation
	get duration()  @ "xs_readgif_get_duration";
	get frameBounds()  @ "xs_readgif_get_frameBounds";
	get frameCount()  @ "xs_readgif_get_frameCount";
	get frameDuration()  @ "xs_readgif_get_frameDuration";
	get frameNumber()  @ "xs_readgif_get_frameNumber";
	get frameX()  @ "xs_readgif_get_frameX";
	get frameY()  @ "xs_readgif_get_frameY";
	get frameWidth()  @ "xs_readgif_get_frameWidth";
	get frameHeight()  @ "xs_readgif_get_frameHeight";
	
	// transparency
	get transparent()  @ "xs_readgif_get_transparent";
	get transparentColor()  @ "xs_readgif_get_transparentColor";
	set transparentColor(value)  @ "xs_readgif_set_transparentColor";

	get ready() @ "xs_readgif_get_ready";
	set available(value) @ "xs_readgif_set_available";
}

export default ReadGIF;
