/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

// The two I2S masters share BCLK/WS and the codec. Use one direction at a time.
export default class AudioResource {
	static #owner;

	static checkAvailable() {
		if (this.#owner)
			throw new Error("close the active M5StickS3 audio input or output first");
	}

	static acquire(audio) {
		this.#owner = audio;
	}

	static release(audio) {
		if (this.#owner === audio)
			this.#owner = undefined;
	}
}
