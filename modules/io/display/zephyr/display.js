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

class Display extends Native("xs_display_destructor") {
	constructor(options) { super(); native("xs_display_constructor").call(this, options); }
	close() { return native("xs_display_close").call(this); }

	configure(options) { return native("xs_display_configure").call(this, options); }
	get configuration() { return native("xs_display_configuration").call(this); }

	begin(options) { return native("xs_display_begin").call(this, options); }
	send(buffer) { return native("xs_display_send").call(this, buffer); }
	end() { return native("xs_display_end").call(this); }

	adapInvalid(options) { return native("xs_display_adaptInvalid").call(this, options); }

	get width() { return native("xs_display_get_width").call(this); }
	get height() { return native("xs_display_get_height").call(this); }
}

export default Display;
