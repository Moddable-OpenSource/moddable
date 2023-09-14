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

export default class Control {
	#former;
	static #onMessage(message) {
		let current = this.focus;
		if (current) {
			const json = JSON.parse(message);
			while (current) {
				current.onJSON(json);
				current = current.#former;
			}
		}
	}
	constructor(options) {
		if (!screen.focus) {
			screen.onMessage = Control.#onMessage;
		}
		this.#former = screen.focus;
		screen.focus = this;
	}
	close() {
		let current = screen.focus;
		let former = null;
		while (current) {
			if (current == this) {
				if (former)
					former.#former = current.#former;
				else
					screen.focus = current.#former;
				break;
			}
			former = current;
			current = current.#former;
		}
	}
	onJSON(json) {
	}
	postJSON(json) {
		screen.postMessage(JSON.stringify(json));
	}
};
