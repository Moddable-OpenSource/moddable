/*
 * Copyright (c) 2019 Moddable Tech, Inc.
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
	NXP Xtrinsic MAG3110 Magnetometer
            Datasheet: 			https://www.nxp.com/docs/en/data-sheet/MAG3110.pdf
*/

import SMBus from "pins/smbus";
import Timer from "timer";

const REGISTERS = {
    OUT_X: 0x01, //big endian
    OUT_Y: 0x03,
    OUT_Z: 0x05,
    WHO_AM_I: 0x07,
    CTRL_REG1: 0x10,
    CTRL_REG2: 0x11
}
Object.freeze(REGISTERS);

const EXPECTED_WHO_AM_I = 0xC4;

class SMBIgnoreErrors extends SMBus { //SMBus implementation that skips reads without throwing if the peripheral does not respond to the command write.
    constructor(dictionary) {
        super(dictionary);
    }
    readByte(register) {
        let test = super.write(register, false);
        if (test) return undefined;
        return super.read(1)[0];
    }
    readBlock(register, count, buffer) {
        let test = super.write(register, false);
        if (test) return undefined;
        return buffer ? super.read(count, buffer) : super.read(count);
    }
}

class Magnetometer extends SMBIgnoreErrors {
    constructor(dictionary) {
        super(Object.assign({ address: 0x0E, throw: false}, dictionary));
        this.checkIdentification();
        this.calibration = { maxX: Number.NEGATIVE_INFINITY, maxY: Number.NEGATIVE_INFINITY, maxZ: Number.NEGATIVE_INFINITY, minX: Number.POSITIVE_INFINITY, minY: Number.POSITIVE_INFINITY, minZ: Number.POSITIVE_INFINITY };
        this.rawValues = new ArrayBuffer(6);
        this.view = new DataView(this.rawValues);
        this.enable();
    }

    checkIdentification() {
        let mID = this.readByte(REGISTERS.WHO_AM_I);
        if (mID != EXPECTED_WHO_AM_I) throw ("bad WHO_AM_I ID for MAG-3110");
    }

    configure(dictionary) {

    }

    calibrate(x, y, z){
        let cal = this.calibration;
        if (x < cal.minX) cal.minX = x;
        if (x > cal.maxX) cal.maxX = x;
        if (y < cal.minY) cal.minY = y;
        if (y > cal.maxY) cal.maxY = y;
        if (z > cal.maxZ) cal.maxZ = z;
        if (z < cal.minZ) cal.minZ = z;
        cal.xSpread = cal.maxX - cal.minX;
        cal.ySpread = cal.maxY - cal.minY;
        cal.zSpread = cal.maxZ - cal.minZ;
        cal.xMid = cal.minX + (cal.xSpread / 2);
        cal.yMid = cal.minY + (cal.ySpread / 2);
        cal.zMid = cal.minZ + (cal.zSpread / 2);
        if (cal.xSpread >= 20 && cal.ySpread >= 20 && cal.zSpread >= 20) return true;
        return false;
    }

    enable() {
        this.writeByte(REGISTERS.CTRL_REG1, 0);
        this.writeByte(REGISTERS.CTRL_REG2, 0b10110000);
        Timer.delay(100);
        this.writeByte(REGISTERS.CTRL_REG1, 0b00000001);
    }

    disable() {
        this.writeByte(REGISTERS.CTRL_REG1, 0);
    }

    sample() {
        let test = this.readBlock(REGISTERS.OUT_X, 6, this.rawValues);
        if (test === undefined) return undefined;
        const x = this.view.getInt16(0);
        const y = this.view.getInt16(2);
        const z = this.view.getInt16(4);

        const calibrated = this.calibrate(x, y, z);

        if (!calibrated) return undefined;

        return {
            x: (x - this.calibration.xMid) * (2000 / this.calibration.xSpread),
            y: (y - this.calibration.yMid) * (2000 / this.calibration.ySpread),
            z: (z - this.calibration.zMid) * (2000 / this.calibration.zSpread)
        }
    }
}

export default Magnetometer;