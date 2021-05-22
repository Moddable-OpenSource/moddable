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

import PWM from "embedded:io/pwm";

const target_period = 18;
const degree0 = 0.5;
const degree180 = 2.0;

const servo = new PWM({
   pin: 5,
   hz: Math.floor(1000 / target_period)
});

const range = (2 ** servo.resolution) - 1;
const minValue = Math.ceil((degree0 / target_period) * range);
const maxValue = Math.floor((degree180 / target_period) * range);

let value = minValue;
let step = 2;

System.setInterval(() => {
   value += step;

   if (value < minValue || value > maxValue){
      step *= -1;
      value += step;
   }
   servo.write(value);
}, 50);
