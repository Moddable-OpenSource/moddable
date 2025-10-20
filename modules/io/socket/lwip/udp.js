/*
 * Copyright (c) 2019-2025  Moddable Tech, Inc.
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

class UDP extends Native("xs_udp_destructor") {
	constructor(dictionary) { super(); native("xs_udp_constructor").call(this, dictionary); };
	close() { return native("xs_udp_close").call(this); }
	read() { return native("xs_udp_read").call(this); }
	write() { return native("xs_udp_write").call(this); }
	add() { return native("xs_udp_add").call(this); }
	remove() { return native("xs_udp_remove").call(this); }

	get format() {
		return "buffer";
	}
	set format(value) {
		if ("buffer" !== value)
			throw new RangeError;
	}
}
Object.freeze(UDP.prototype);

export default UDP;
