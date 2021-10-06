/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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

const led1 = new device.io.PWM({
   pin: device.pin.led
});

const led2 = new device.io.PWM({
   pin: 5
});

const range = (2 ** led1.resolution) - 1;

let value = 0;
let step = 5;

System.setInterval(() => {
   value += step;

   if (value < 0 || value > range) {
      step *= -1;
      value += step;
   }
   led1.write(value);
   led2.write(range - value);
}, 5);
