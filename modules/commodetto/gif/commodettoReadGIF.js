/*
* Copyright (c) 2021-2025  Moddable Tech, Inc.
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

class ReadGIF extends Native("xs_readgif_destructor") {
	constructor(buffer) { super(); native("xs_readgif").call(this, buffer); }
	close() { return native("xs_readgif_close").call(this); }
	first() { return native("xs_readgif_first").call(this); }
	next() { return native("xs_readgif_next").call(this); }
	// bitmap
	get width() { return native("xs_readgif_get_width").call(this); }
	get height() { return native("xs_readgif_get_height").call(this); }
	get pixelFormat() { return native("xs_readgif_get_pixelFormat").call(this); }
	get offset() {
		return 0;
	}
	// animation
	get duration() { return native("xs_readgif_get_duration").call(this); }
	get frameBounds() { return native("xs_readgif_get_frameBounds").call(this); }
	get frameCount() { return native("xs_readgif_get_frameCount").call(this); }
	get frameDuration() { return native("xs_readgif_get_frameDuration").call(this); }
	get frameNumber() { return native("xs_readgif_get_frameNumber").call(this); }
	get frameX() { return native("xs_readgif_get_frameX").call(this); }
	get frameY() { return native("xs_readgif_get_frameY").call(this); }
	get frameWidth() { return native("xs_readgif_get_frameWidth").call(this); }
	get frameHeight() { return native("xs_readgif_get_frameHeight").call(this); }
	// transparency
	get transparent() { return native("xs_readgif_get_transparent").call(this); }
	get transparentColor() { return native("xs_readgif_get_transparentColor").call(this); }
	set transparentColor(value) { native("xs_readgif_set_transparentColor").call(this, value); }
	get ready() { return native("xs_readgif_get_ready").call(this); }
	set available(value) { native("xs_readgif_set_available").call(this, value); }
}
export default ReadGIF;
