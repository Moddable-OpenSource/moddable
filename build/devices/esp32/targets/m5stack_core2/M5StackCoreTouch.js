/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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
 
 import Touch from "ft6206"

class M5StackCoreTouch extends Touch {
	#captured;		// undefined or pressed button instance

	read(points) {
		super.read(points);

		if (this.#captured) {
			if (0 === points[0].state) {
				this.#captured.write(0);
				this.#captured = undefined;
			}
		}
		else if ((1 === points[0].state) && (points[0].y >= 240)) {
			this.#captured = button[String.fromCharCode('a'.charCodeAt() + Math.idiv(points[0].x, 107))];
			this.#captured?.write(1);
		}

		if (this.#captured)
			points[0].state = 0;
	}
}

export default M5StackCoreTouch;
