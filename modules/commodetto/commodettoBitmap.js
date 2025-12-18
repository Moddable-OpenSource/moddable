/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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


class Bitmap extends Native("xs_Bitmap_destructor") {
	constructor(width, height, format, buffer, offset) { super(); native("xs_Bitmap").call(this, width, height, format, buffer, offset); }
	get width() { return native("xs_bitmap_get_width").call(this); }
	get height() { return native("xs_bitmap_get_height").call(this); }
	get pixelFormat() { return native("xs_bitmap_get_pixelFormat").call(this); }
	get offset() { return native("xs_bitmap_get_offset").call(this); }
	set buffer(buffer) { native("xs_bitmap_set_buffer").call(this, buffer); }
	get buffer() { return native("xs_bitmap_get_buffer").call(this); }
	static depth(pixelFormat) { return native("xs_bitmap_get_depth").call(this, pixelFormat); }
}

Bitmap.Default = 1;
Bitmap.Monochrome = 3;
Bitmap.Gray16 = 4;
Bitmap.Gray256 = 5;
Bitmap.RGB332 = 6;
Bitmap.RGB565LE = 7;
Bitmap.RGB565BE = 8;
Bitmap.RGB24 = 9;
Bitmap.RGBA32 = 10;
Bitmap.CLUT16 = 11;
Bitmap.ARGB4444 = 12;
Bitmap.RGB444 = 13;
Bitmap.BGRA32 = 14;
Bitmap.JPEG = 15;
Bitmap.PNG = 16;
Bitmap.CLUT256 = 17;
Bitmap.CLUT32 = 18;
Bitmap.YUV422 = 20;
Bitmap.MonochromeAligned = 21;
Bitmap.Pebble = 22;

Bitmap.RLE = 0x80;	// flag applied to pixel types

export default Bitmap;
