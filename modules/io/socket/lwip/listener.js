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

import TCP from "embedded:io/socket/tcp"

class Listener extends Native("xs_listener_destructor_") {
	constructor(options) { super(); native("xs_listener_constructor").call(this, options); };
	close() { return native("xs_listener_close_").call(this); }
	read() {
		return native("xs_listener_read").call(this, new TCP);
	}

	get port() { return native("xs_listener_get_port").call(this); };
	get format() {
		return "socket/tcp";
	}
	set format(value) {
		if ("socket/tcp" !== value)
			throw new RangeError;
	}
}

export default Listener;
