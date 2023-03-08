/*
 * Copyright (c) 2023 Moddable Tech, Inc.
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

import Matter from "matter";

const led = new device.peripheral.led.Default;

let x = new Matter({
    onChanged: function(value) {
        led.on = value/255;
        trace(`Light brightness changed to: ${value}\n`)
    }
});
