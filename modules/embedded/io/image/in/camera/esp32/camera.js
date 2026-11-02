/*
 * Copyright (c) 2024-2026  Moddable Tech, Inc.
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

class DisposableHostBuffer extends Native("_xs_disposable_hostbuffer_destructor"){
	close() { return native("_xs_disposable_hostbuffer_close").call(this); }

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

// A board that shares the camera SCCB puts the endpoint on device.i2c.camera.
// The instance stays open for the life of the camera so other devices join its bus.
function openCameraBus(options) {
	let sensor = options?.sensor;
	if (!sensor) {
		const board = globalThis.device?.i2c?.camera;
		if (!board)
			return;
		sensor = {
			...globalThis.device.i2c.default,
			io: globalThis.device.io.I2C,
			hz: 400_000,
			...board
		};
	}
	if (!sensor.io)
		return;
	return new sensor.io({
		hz: 400_000,
		...sensor
	});
}

class Camera extends Native("xs_camera_destructor") {
	#i2c;

	constructor(options) {
		super();
		const i2c = openCameraBus(options);
		const nativeOptions = {...options, prototype: DisposableHostBuffer.prototype};
		if (i2c)
			nativeOptions.i2c = i2c;
		try {
			native("xs_camera_constructor").call(this, nativeOptions);
		}
		catch (e) {
			i2c?.close();
			throw e;
		}
		this.#i2c = i2c;
	}
	close() {
		native("xs_camera_close").call(this);
		this.#i2c?.close();
		this.#i2c = undefined;
	}
	read(samples) { return native("xs_camera_read").call(this, samples); };
	start() { return native("xs_camera_start").call(this); };
	stop() { return native("xs_camera_stop").call(this); };

	get format() { return native("xs_camera_get_format").call(this); };
	set format(it) { native("xs_camera_set_format").call(this, it); };
	get imageType() { return native("xs_camera_get_imageType").call(this); };
	get width() { return native("xs_camera_get_width").call(this); };
	get height() { return native("xs_camera_get_height").call(this); };

	get identification() { return native("xs_camera_get_identification").call(this); };
	get configuration() { return native("xs_camera_get_configuration").call(this); };
	configure(options) { return native("xs_camera_configure").call(this, options); };

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export default Camera;
