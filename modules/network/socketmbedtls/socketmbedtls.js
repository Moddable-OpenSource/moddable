/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

/*
	socket mbledtls
*/

export class SocketTLS @ "xs_socketmbedtls_destructor" {
	constructor(dictionary) @ "xs_socketmbedtls";		// {address: "IPv4.0.1.2", host: "foo.com", port: 80, kind: "TCP/UDP"}

	read() @ "xs_socketmbedtls_read";
	write() @ "xs_socketmbedtls_write";

	close() @ "xs_socketmbedtls_close";

	// callback()		// connected, error/disconnected, read ready (bytes ready), write ready (bytes can write)
};

export class ListenerTLS @ "xs_listenermbedtls_destructor" {
	constructor(dictionary) @ "xs_listenermbedtls";		// {port: 80}

	close() @ "xs_listenermbedtls_close";

	// callback()		// new socket arrived
};

export default {
	SocketTLS,
	ListenerTLS
};


let
certificate,
verify
;
