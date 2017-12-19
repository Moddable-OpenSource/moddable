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


import CLI from "cli";
import WiFi from "wifi";

class ESP {
	static reset(what) @ "xs_esp_reset";
	static get phy() @ "xs_esp_phy_get";
	static set phy() @ "xs_esp_phy_set";
}

CLI.install(function(command, parts) {
	switch (command) {
		case "esp":
			switch (parts.shift().toLowerCase()) {
				case "reset":
					ESP.reset();
					break;
				case "mode":
					if (0 === parts.length) {
						switch (WiFi.mode) {
							case 0:		this.line("null"); break;
							case 1:		this.line("station"); break;
							case 2:		this.line("softap"); break;
							case 3:		this.line("stationap"); break;
							default:	this.line("(unknown)"); break;
						}
					}
					else {
						switch (parts.shift().toLowerCase()) {
							case "null":		WiFi.mode = 0; break;
							case "station":		WiFi.mode = 1; break;
							case "softap":		WiFi.mode = 2; break;
							case "stationap":	WiFi.mode = 3; break;
							default:	throw new Error("unknown mode");
						}
					}
					break;
				case "phy":
					if (0 === parts.length)
						this.line(ESP.phy);
					else
						ESP.phy = parts[0];
					break;
				default:
					return false;
			}
			break;

		case "help":
			this.line("esp reset - reset stored Wi-Fi settings");
			this.line("esp mode - display Wi-Fi mode");
			this.line("esp mode [null, station, softap, stationap] - set Wi-Fi mode");
			this.line("esp phy - display Wi-Fi PHY mode");
			this.line("esp phy [b, g, n] - set Wi-Fi PHY mode");
			break;

		default:
			return false;
	}

	return true;
});
