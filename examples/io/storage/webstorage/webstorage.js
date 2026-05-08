/*
 * Copyright (c) 20252-2026  Moddable Tech, Inc.
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
 
/* 
	An implementation of the Web's Storage feature using ECMA-419 Key-Value Storage

	See:
		https://developer.mozilla.org/en-US/docs/Web/API/Window/localStorage
		https://ecmatc53.github.io/spec/web/spec.html#-26-persistent-storage-key-value
*/

 class WebStorage {
	#kvp;

	constructor(kvp) {
		this.#kvp = kvp;
		this.#kvp.format = "string";
	}
	get length() {
		let length = 0;
		for (let _ of this.#kvp)
			length++;
		return length;
	}
	key(index) {
		index = Math.trunc(index);
		if ((index < 0) || Number.isNaN(index))
			return null;
		for (let key of this.#kvp) {
			if (0 === index--)
				return key;
		}
		return null;
	}
	getItem(key) {
		return this.#kvp.read(key) ?? null;
	}
	setItem(key, value) {
		this.#kvp.write(key, value);
	}
	remove(key) {
		trace("localStorage.remove()  deprecated. Use localStorage.removeItem() instead\n");
		return this.removeItem(key);
	}
	removeItem(key) {
		this.#kvp.delete(key);
	}
	clear() {
		Array.from(this.#kvp).forEach(key => this.#kvp.delete(key));
	}
}

export default WebStorage;
