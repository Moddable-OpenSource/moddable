/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

import Timer from "timer";

const timerInterval = 50;

let state = "idle";
let timer;
let scanning = false;

export default class WiFi {
    #onNotify;

    constructor(dictionary, onNotify) {
        if (timer)
            throw new Error("only one instance in simulator");

        this.onNotify = onNotify;

        timer = Timer.repeat(() => {
            switch (state) {
                case "starting":
                    state = WiFi.connected;
                    this.#onNotify(state);
                    break;

                case WiFi.connected:
                    state = WiFi.gotIP;
                    this.#onNotify(state);
                    break;
            }
        }, timerInterval);

        if (dictionary){
            WiFi.connect(dictionary);
        }
    }
    close() {
        if (timer) {
            Timer.clear(timer)
            timer = undefined;
        }
    }
    set onNotify(value) {
        this.#onNotify = value ?? function () { };
    }
    static set mode(value) { 
        if (value != 1) throw new Error("AP mode is not implemented in the simulator");
    }
    static get mode() { return 1; }
    static scan(dictionary, callback) {
		if (scanning)
			throw new Error("already scanning");

		scanning = true;
		const items = accessPoints.slice();
		Timer.set(id => {
			if (items.length)
				callback({...items.shift()});
			else {
				Timer.clear(id);
				scanning = false;
				callback();
			}
		}, 1500, 50);
     }
    static connect(dictionary) {
        if (!dictionary) {
            state = "ending";
            return;
        }

        state = "starting";
        if (timer)
            Timer.schedule(timer, timerInterval, timerInterval);
    }
    static accessPoint(dictionary) {
        throw new Error("AP mode is not implemented in the simulator");
    }
    static close() { WiFi.connect(); }
}
WiFi.gotIP = "gotIP";
WiFi.lostIP = "lostIP";
WiFi.connected = "connect";
WiFi.disconnected = "disconnect";



const accessPoints = [
   {
      "ssid": "468 8th - slow",
      "rssi": -84,
      "channel": 3,
      "hidden": false,
      "authentication": "wpa2_psk"
   },
   {
      "ssid": "JAMS2",
      "rssi": -72,
      "channel": 6,
      "hidden": false,
      "authentication": "wpa2_psk"
   },
   {
      "ssid": "xfinitywifi",
      "rssi": -79,
      "channel": 6,
      "hidden": false,
      "authentication": "none"
   },
   {
      "ssid": "baja",
      "rssi": -81,
      "channel": 8,
      "hidden": false,
      "authentication": "wpa2_psk"
   },
   {
      "ssid": "baja",
      "rssi": -88,
      "channel": 8,
      "hidden": false,
      "authentication": "wpa2_psk"
   },
   {
      "ssid": "Breathe",
      "rssi": -85,
      "channel": 9,
      "hidden": false,
      "authentication": "wpa2_psk"
   },
   {
      "ssid": "Breathe-Guest",
      "rssi": -84,
      "channel": 9,
      "hidden": false,
      "authentication": "wpa2_psk"
   },
   {
      "ssid": "468 8th - slow",
      "rssi": -23,
      "channel": 10,
      "hidden": false,
      "authentication": "wpa2_psk"
   },
   {
      "ssid": "HOME-608A_2GEXT",
      "rssi": -86,
      "channel": 11,
      "hidden": false,
      "authentication": "wpa_wpa2_psk"
   },
   {
      "ssid": "ATTePGQDkI",
      "rssi": -87,
      "channel": 11,
      "hidden": false,
      "authentication": "wpa2_psk"
   },
   {
      "ssid": "LBBSB",
      "rssi": -85,
      "channel": 11,
      "hidden": false,
      "authentication": "wpa2_psk"
   }
];
Object.freeze(accessPoints, true);

