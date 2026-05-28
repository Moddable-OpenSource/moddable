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

class WiFi extends Native("xs_wifi_destructor"){
	constructor(options) { super(); native("xs_wifi").call(this, options); };

	close() { return native("xs_wifi_close").call(this); }
	scan(options) { return native("xs_wifi_scan").call(this, options); }
	connect(options) { return native("xs_wifi_connect").call(this, options); }
	disconnect() { return native("xs_wifi_disconnect").call(this); }
	configure(options) { return native("xs_wifi_configure").call(this, options); }

	get connection() { return native("xs_wifi_connection_get").call(this); }
	get address() { return native("xs_wifi_address_get").call(this); }
	get MAC() { return native("xs_wifi_MAC_get").call(this); }
	get SSID() { return native("xs_wifi_SSID_get").call(this); }
	get BSSID() { return native("xs_wifi_BSSID_get").call(this); }
	get RSSI() { return native("xs_wifi_RSSI_get").call(this); }
	get channel() { return native("xs_wifi_channel_get").call(this); }
};

export default WiFi;

