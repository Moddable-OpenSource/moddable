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

class TCP @ "xs_tcp_destructor" {
	constructor(dictionary) @ "xs_tcp_constructor";
	close() @ "xs_tcp_close"
	read() @ "xs_tcp_read"
	write(byte) @ "xs_tcp_write"

	get remoteAddress() @ "xs_tcp_get_remoteAddress"
	get remotePort() @ "xs_tcp_get_remotePort"

	get format() @ "xs_tcp_get_format"
	set format() @ "xs_tcp_set_format"
}
Object.freeze(TCP.prototype);

export default TCP;
