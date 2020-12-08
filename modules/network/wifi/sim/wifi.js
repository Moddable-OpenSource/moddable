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
        throw new Error("Wi-Fi scan is not implemented in the simulator")
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