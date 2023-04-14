/*
 * Copyright (c) 2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Sensor from "embedded:sensor/Oxygen/SEN0322";
import Timer from "timer";

const knownOxygenVal = 20.9; // Known concentration of oxygen in the air, for calibration
const oxygenMV = 0; // The value marked on the sensor, set to 0 unless otherwise required

const sensor = new Sensor({
    sensor: {
        ...device.I2C.default,
        io: device.io.I2C
    }
});

sensor.configure({
    vol: knownOxygenVal,
    mv: oxygenMV
});

Timer.repeat(() =>
{
    const sample = sensor.sample();
    trace(`O2: ${sample.O} ppm\n`);
}, 500);
