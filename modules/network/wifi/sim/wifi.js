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

import Timer from "timer";

const timerInterval = 1000;

let state = "idle";
let state_next = "idle";
let timer;
let scanning = false;
let ap_mode=1;
let pwd = '';

export default class WiFi {
    #onNotify;

    constructor(dictionary, onNotify) {
        if (timer)
            throw new Error("only one instance in simulator");

        this.onNotify = onNotify;

        timer = Timer.repeat(() => {
            if ( state_next != state) {
                //trace(`Wifi: ${state} -> ${state_next}\n`);
                state=state_next;
            }
            switch (state) {
                case "starting":
                    // simulate invalid password
                    if ( pwd === 'invalid!')
                        state_next = 'ending'
                    else
                        state_next = WiFi.connected
                    break;

                case WiFi.connected:
                    this.#onNotify(state);
                    state_next = WiFi.gotIP;
                    break;
                case WiFi.gotIP:
                    this.#onNotify(state);
                    state_next = 'done';
                    break;
                case 'ending': {
                    state_next = WiFi.disconnected;
                    break;
                }
                case 'done': {
                    if (timer) {
                        Timer.clear(timer)
                        timer = undefined;
                    }
                }
                case WiFi.disconnected: {
                    this.#onNotify(state);
                    if (timer) {
                        Timer.clear(timer)
                        timer = undefined;
                    }
                    state_next = 'idle';
                }
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
        ap_mode=value;
    }
    static get mode() { return ap_mode; }
    static scan(dictionary, callback) {
		if (scanning)
			throw new Error("already scanning");

		scanning = true;
		const items = accessPoints.slice();
        let total=items.length+1;
		Timer.set(id => {
			if (items.length) {

				callback({...items.shift()});
                let delay=10*(total-items.length);
                Timer.schedule(id,10,delay);
            }
			else {
				Timer.clear(id);
				scanning = false;
				callback();
			}
		}, 1500 );
     }
    static connect(dictionary) {
        if (!dictionary) {
            state = "ending";
            return;
        }

        state_next = "starting";
        pwd=dictionary?.password;
        if (timer)
            Timer.schedule(timer, timerInterval, timerInterval);
    }
    static accessPoint(dictionary) {
        trace(`Sim AP started ssid: ${dictionary.ssid}\n`);
    }
    static close() { WiFi.connect(); }
	
	static disconnect() {
		state = WiFi.disconnected;
	}
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
