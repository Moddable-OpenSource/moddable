/*
 * Copyright (c) 2024  Moddable Tech, Inc.
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

class DisposableHostBuffer @ "_xs_disposable_hostbuffer_destructor"{
	close() @ "_xs_disposable_hostbuffer_close"
}
// DisposableHostBuffer.prototype[Symbol.dispose] = DisposableHostBuffer.prototype.close;

function constructor(options) @ "xs_camera_constructor";

class Camera @ "xs_camera_destructor" {
	constructor(options) {
		constructor.call(this, {...options, prototype: DisposableHostBuffer.prototype});
	} 
	close() @ "xs_camera_close";
	read(samples) @ "xs_camera_read";
	start() @ "xs_camera_start";
	stop() @ "xs_camera_stop";
	
	get format() @ "xs_camera_get_format";
	set format(it) @ "xs_camera_set_format";
	get imageType() @ "xs_camera_get_imageType";
	get width() @ "xs_camera_get_width";
	get height() @ "xs_camera_get_height";
}

export default Camera;
