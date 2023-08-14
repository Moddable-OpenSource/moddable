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
	#on;
	#variant;
	constructor(options = {}) {
		super();
		this.#variant = options.variant ?? 0;
		this.#on = options.invert ? 0 : 1;
	}
	read() {
		const value = screen.readLED();
		return this.#on ? value : 1 - value;
	}
	write(value) {
		this.postJSON({led:this.#on ? value : 1 - value, variant:this.#variant});
	}
	on() {
		this.write(this.#on);
	}
	off() {
		this.write(1 - this.#on);
	}
}
