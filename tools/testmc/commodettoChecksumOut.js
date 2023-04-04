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

import Bitmap from "commodetto/Bitmap";
import {Digest} from "crypt";

export default class ChecksumOut {
	#digest = new Digest("MD5");
	#continue;
	#full;
	#show;
	#screen = globalThis.screen;

	constructor(options) {
		Object.defineProperty(this, "width", {value: options.width});
		Object.defineProperty(this, "height", {value: options.height});
		Object.defineProperty(this, "pixelFormat", {value: options.pixelFormat ?? Bitmap.RGB565LE});
	}
	begin(x, y, width, height) {
		this.#show?.begin(x, y, width, height);

		if (!this.#continue)
			this.#digest.reset();
		this.#continue = false;
		
		this.#digest.write(Int32Array.of(x, y, width, height).buffer);
	}
	send(pixels, offset, byteLength) {
		this.#show?.send(pixels, offset, byteLength);

		if (offset || (byteLength !== pixels.byteLength))
			pixels = new Uint8Array(pixels, offset, byteLength);
		
		this.#digest.write(pixels);
	}
	end() {
		this.#show?.end();

		const bytes = new Uint8Array(this.#digest.close());
		const checksum = [];
		for (let i = 0, length = bytes.length; i < length; i++)
			checksum.push(bytes[i].toString(16).padStart(2, "0"));
		this.checksum = checksum.join("");
		this.#continue = false;
	}
	continue() {
		if (this.#show)
			this.#show.continue();

		this.#continue = true
	}
	adaptInvalid(r) {
		if (this.#full)
			r.set(0, 0, this.width, this.height);
		else
			this.#show?.adaptInvalid(r);
	}
	pixelsToBytes(count) {
		return ((Bitmap.depth(this.pixelFormat) * count) + 7) >> 3;
	}
	
	configure(options) {
		if ("full" in options)
			this.#full = options.full;
		if ("show" in options)
			this.#show = options.show ? this.#screen : null;
	}
}
