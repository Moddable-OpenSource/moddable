/*
 * Copyright (c) 2016-2023 Moddable Tech, Inc.
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

import config from "mc/config";
import Time from "time";
import Ethernet from "ethernet";
import Net from "net";
import SNTP from "sntp";

export default function (done) {	

    if (config.ssid !== undefined) {
		trace("Skipping Ethernet setup because Wi-Fi SSID is configured.\n");
		return done();
	}

    try {
        Ethernet.start();
    } catch (error) {
        trace(`Ethernet hardware not found.\n`);
        return done();
    }

    globalThis.ethernet = {connected: false};
    trace(`Waiting for Ethernet link.\n`);
    let monitor = new Ethernet(function (msg, code) {
        switch (msg) {
            case "gotIP":
                trace(`IP address ${Net.get("IP", "ethernet")}\n`);
                trace(`MAC address: ${Net.get("MAC", "ethernet")}\n`);
                ethernet.connected = true;

                if (!config.sntp)
                    return done();

                new SNTP({host: config.sntp}, function(message, value) {
                    if (1 === message) {
                        trace("got time\n");
                        Time.set(value);
                    }
                    else if (message < 0)
                        trace("can't get time\n");
                    else
                        return;
                    done();
                });
                break;

            case "connect":
                trace(`Ethernet connected\n`);
                break;

            case "disconnect":
                ethernet.connected = false;
                trace(`Ethernet disconnected\n`);
                break;
        }
    });	
}
