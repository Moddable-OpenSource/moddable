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
	#onPush;
	#pressed;
	#buttonKey;
	constructor(options) {
		super();
		this.#onPush = options.onPush ?? this.onPush;
		this.#buttonKey = options.buttonKey ?? "button";
		if (options.target)
			this.target = options.target;
	}
	onJSON(json) {
		const pressed = json[this.#buttonKey];
		if (undefined === pressed)
			return;
		this.#pressed = pressed; 
		this.#onPush(pressed);
	}
	read() {
		return this.#pressed;
	}
	get pressed() {
		return this.#pressed;
	}
};
