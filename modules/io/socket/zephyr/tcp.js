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

class TCP extends Native("xs_tcp_destructor") {
	constructor(dictionary) { super(); native("xs_tcp_constructor").call(this, dictionary); };
	close() { return native("xs_tcp_close").call(this); }
	read() { return native("xs_tcp_read").call(this); }
	write(byte) { return native("xs_tcp_write").call(this, byte); }

	get remoteAddress() { return native("xs_tcp_get_remoteAddress").call(this); }
	get remotePort() { return native("xs_tcp_get_remotePort").call(this); }

	get format() { return native("xs_tcp_get_format").call(this); }
	set format(it) { native("xs_tcp_set_format").call(this, it); }
}

export default TCP;
