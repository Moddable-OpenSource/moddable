/*
 * Copyright (c) 2022-2025  Moddable Tech, Inc.
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

import I2C from "embedded:implementation/i2csync"

I2C.Async = class extends Native("_xs_i2casync_destructor") {
	constructor(options) { super(); native("_xs_i2casync_constructor").call(this, options); }
	close() { return native("_xs_i2casync_close").call(this); }
	read(count) { return native("_xs_i2casync_read").call(this, count); }
	write(buffer) { return native("_xs_i2casync_write").call(this, buffer); }
}

export default I2C;
