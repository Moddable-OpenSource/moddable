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
	socket
*/

export class Socket @ "xs_socket_destructor" {
	constructor(dictionary) @ "xs_socket";		// {address: "IPv4.0.1.2", host: "foo.com", port: 80, kind: "TCP/UDP"}

	read() @ "xs_socket_read";
	write() @ "xs_socket_write";

	close() @ "xs_socket_close";

	// callback()		// connected, error/disconnected, read ready (bytes ready), write ready (bytes can write)
};

export class Listener @ "xs_listener_destructor" {
	constructor(dictionary) @ "xs_listener";		// {port: 80}

	close() @ "xs_listener_close";

	// callback()		// new socket arrived
};

Object.freeze(Socket.prototype);
Object.freeze(Listener.prototype);

export default {
	Socket,
	Listener
};
