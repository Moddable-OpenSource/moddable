/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
import Timer from "timer";
import Net from "net";
import WiFi from "wifi";

CLI.install(function(command, parts) {
	if ("help" === command)
		;
	else if ("wifi" === command)
		command = parts.shift();
	else
		return false;

	switch (command) {
		case "net":
			this.line(`  IP Address: ${Net.get("IP")}`);
			this.line(`  SSID: ${Net.get("SSID")}`);
			this.line(`  BSSID: ${Net.get("BSSID")}`);
			this.line(`  RSSI: ${Net.get("RSSI")}`);
			this.line(`  MAC Address: ${Net.get("MAC")}`);
			break;

		case "connect": {
			let dictionary;
			if (1 === parts.length)
				dictionary = {ssid: parts[0]};
			else if (2 === parts.length)
				dictionary = {ssid: parts[0], password: parts[1]};
			else
				throw new Error("bad arguments");

			this.line(`Trying to connect to "${dictionary.ssid}"...`)
			this.suspend();
			this.monitor = new WiFi(dictionary, msg => {
				if (WiFi.gotIP === msg) {
					this.line(`  IP address ${Net.get("IP")}`);

					if (this.monitor.timer)
						Timer.clear(this.monitor.timer);
					this.monitor.close();
					delete this.monitor;
					this.resume();
				}
				else if (WiFi.connected === msg)
					this.line(`  Wi-Fi connected to "${Net.get("SSID")}"`);
			});
			this.monitor.timer = Timer.set(() => {
				delete this.monitor.timer;
				this.line("  * connect timed out *")
				this.monitor.close();
				delete this.monitor;
				this.resume();
			}, 15 * 1000);
			}
			break;

		case "disconnect":
			WiFi.disconnect();
			break;

		case "scan":
			let cancel = false;
			let header = false;
			WiFi.scan({hidden: true}, ap => {
				if (cancel)
					return;
				if (ap) {
					if (!header) {
						this.more = true;
						this.line("SSID".padEnd(32), "Security".padEnd(20), "RSSI".padEnd(10), "Channel".padEnd(10), "Hidden".padEnd(10));
						header = true;
					}
					this.line(ap.ssid.padEnd(32), (ap.authentication ?? "").padEnd(20), ap.rssi.toString().padEnd(10), ap.channel.toString().padEnd(10), ap.hidden.toString());
				}
				else {
					this.resume();
					this.more = false;
				}

			});
			this.suspend(() => cancel = true);
			break;

		case "help":
			this.line("wifi connect ssid [password] - connect to network ssid using optional password");
			this.line("wifi disconnect - disconnect from Wi-Fi network");
			this.line("wifi net - show network configuration including SSID, MAC, and IP");
			this.line("wifi scan - list visible Wi-Fi access points");
			break;

		default:
			return false;
	}

	return true;
});
