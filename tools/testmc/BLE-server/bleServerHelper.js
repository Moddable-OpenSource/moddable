/*
 * Copyright (c) 2022 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import BLEServer from "bleserver";
import Timer from "timer";

class ServerHelper extends BLEServer {
    constructor(options) {
        super();

        this.index = options.testIndex;
        this.done = false;

        Timer.set(id => {
            this.onComplete(0, true);
        }, $TESTMC.config.ble.server.timeout);
    }

    onReady() {
        this.deviceName = $TESTMC.config.ble.server.deviceName;

        const advertisingData = {
            flags: 6,
            completeName: this.deviceName,
            manufacturerSpecific: {
                identifier: 0x0e,
                data: [this.index]
            }
        }

        this.startAdvertising({advertisingData});
    }

    onConnected() {
        this.stopAdvertising();
    }

    onDisconnected() {
        trace(`disconnected\n`);
    }

    onCharacteristicWritten(characteristic, value) {
        if ('result' === characteristic.name) {
            this.onComplete(value);
        }
    }

    onComplete(value, timeout = false) {
        if (!this.done) {
            this.done = true;
            this.close();
    
            if (value === "okay") {
                $DONE();
            } else {
                const error = timeout ? "BLE Server timeout" : value ?? "unknown remote test failure";
                $DONE(error);
            }
        }
    }
}

export default ServerHelper;