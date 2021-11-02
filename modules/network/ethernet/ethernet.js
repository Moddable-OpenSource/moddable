/*
 * Copyright (c) 2016-2021 Moddable Tech, Inc.
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
	ethernet
*/

export default class Ethernet @ "xs_ethernet_destructor" {
	constructor(dictionary, onNotify) @ "xs_ethernet_constructor";
	close() @ "xs_ethernet_close";
	set onNotify() @ "xs_ethernet_set_onNotify";

	build(onNotify) {
		if (onNotify)
			this.onNotify = onNotify;
	}

	static start() @ "xs_ethernet_start";
}

Ethernet.gotIP = "gotIP";
Ethernet.lostIP = "lostIP";
Ethernet.connected = "connect";
Ethernet.disconnected = "disconnect";

Object.freeze(Ethernet.prototype);
