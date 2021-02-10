/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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

class Listener @ "xs_listener_destructor_" {
	constructor(dictionary) @ "xs_listener_constructor";
	close() @ "xs_listener_close_"
	read() {
		return read.call(this, new TCP);
	}

	get format() {
		return "socket/tcp";
	}
	set format(value) {
		if ("socket/tcp" !== value)
			throw new RangeError;
	}
}
Object.freeze(Listener.prototype);

function read() @ "xs_listener_read";

export default Listener;
