/*
 * Copyright (c) 2022-2026  Moddable Tech, Inc.
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

import Touch from "embedded:sensor/Touch/FT6x06";

class M5StackCoreTouch extends Touch {
	#captured;		// undefined or pressed button instance
	#capturedID;	// touch id captured as virtual button

	sample() {
		const points = super.sample();
		if (!points)
			return points;

		let capturedPointIndex = -1;

		if (this.#captured) {
			for (let i = 0, length = points.length; i < length; i++) {
				const point = points[i];

				if (point.id === this.#capturedID) {
					capturedPointIndex = i;
					break;
				}
			}

			if (-1 === capturedPointIndex) {
				this.#captured.write(0);
				this.#captured = undefined;
				this.#capturedID = undefined;
			}
		}
		else {
			for (let i = 0, length = points.length; i < length; i++) {
				const point = points[i];

				if (point.y >= 240) {
					const index = Math.idiv(point.x, 107);
					this.#captured = globalThis.button[String.fromCharCode('a'.charCodeAt() + index)];

					if (this.#captured) {
						this.#capturedID = point.id;
						this.#captured.write(1);
						capturedPointIndex = i;
					}

					break;
				}
			}
		}

		if (-1 !== capturedPointIndex) {
			points.splice(capturedPointIndex, 1);
		}

		return points;
	}
}

export default M5StackCoreTouch;
