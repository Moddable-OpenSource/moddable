/*
 * Copyright (c) 2024-2025  Moddable Tech, Inc.
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
class Camera extends Native("xs_camera_destructor") {
	constructor(dictionary) { super(); native("xs_camera_constructor").call(this, dictionary); };
	close() { return native("xs_camera_close").call(this); };
	read(samples) { return native("xs_camera_read").call(this, samples); };
	start() { return native("xs_camera_start").call(this); };
	stop() { return native("xs_camera_stop").call(this); };
	
	get format() { return native("xs_camera_get_format").call(this); };
	set format(it) { native("xs_camera_set_format").call(this, it); };
	get width() { return native("xs_camera_get_width").call(this); };
	get height() { return native("xs_camera_get_height").call(this); };
	get imageType() { return native("xs_camera_get_imageType").call(this); };
}

export default Camera;
