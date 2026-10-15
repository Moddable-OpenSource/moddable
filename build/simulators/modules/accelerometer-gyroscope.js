/*
 * Copyright (c) 2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import Control from "control";
export default class extends Control {
	#accelerometer;
	#gyroscope;
	constructor(options) {
		super(options);
		this.#accelerometer = { x:0, y:0, z:0 };
		this.#gyroscope = { x:0, y:0, z:0 };
	}
	onJSON(json) {
		const accelerometer = json.accelerometer;
		if (accelerometer) {
			this.#accelerometer = accelerometer;
		}
		const gyroscope = json.gyroscope;
		if (gyroscope) {
			this.#gyroscope = gyroscope;
		}
	}
	sample() {
		return {
			accelerometer: { ...this.#accelerometer },
			gyroscope: { ...this.#gyroscope },
		}
	}
};
