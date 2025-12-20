/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

const roundRect = {
	__proto__: Content.prototype,
	_create($, it) { return native("PiuRoundRect_create").call(this, $, it); },
	get corners() { return native("PiuRoundRect_get_corners").call(this); },
	set corners(it) { native("PiuRoundRect_set_corners").call(this, it); },
	get radius() { return native("PiuRoundRect_get_radius").call(this); },
	set radius(it) { native("PiuRoundRect_set_radius").call(this, it); },
};
export const RoundRect = Template(roundRect);
RoundRect.topLeft = 1;
RoundRect.topRight = 2;
RoundRect.bottomLeft = 4;
RoundRect.bottomRight = 8;
Object.freeze(roundRect);
globalThis.RoundRect = RoundRect;
export default RoundRect;