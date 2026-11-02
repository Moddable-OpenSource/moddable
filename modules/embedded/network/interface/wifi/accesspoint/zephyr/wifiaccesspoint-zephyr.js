/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

class Station extends Native("xs_apstation_destructor") {
	constructor() { throw new Error; }

	close() { return native("xs_apstation_close").call(this); }

	get MAC() { return native("xs_apstation_MAC_get").call(this); }
	get address() { return native("xs_apstation_address_get").call(this); }

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

class WiFiAccessPoint extends Native("xs_wifiaccesspoint_destructor") {
	constructor(options) {
		super();
		native("xs_wifiaccesspoint").call(this, options, Station.prototype);
	};

	close() { return native("xs_wifiaccesspoint_close").call(this); }
	configure(options) { return native("xs_wifiaccesspoint_configure").call(this, options); }

	get connection() { return native("xs_wifiaccesspoint_connection_get").call(this); }
	get address() { return native("xs_wifiaccesspoint_address_get").call(this); }
	get MAC() { return native("xs_wifiaccesspoint_MAC_get").call(this); }
	get SSID() { return native("xs_wifiaccesspoint_SSID_get").call(this); }
	get channel() { return native("xs_wifiaccesspoint_channel_get").call(this); }

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
};

export default WiFiAccessPoint;
