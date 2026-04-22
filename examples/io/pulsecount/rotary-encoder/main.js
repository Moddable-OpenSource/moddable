/*
 * Copyright (c) 2021-2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

if (!globalThis.device?.io?.PulseCount)
   throw new Error("No PulseCount constructor");

const signal = undefined;     // set the signal pin
const control = undefined;    // set control signal pin
if ((undefined === signal) || (undefined === control))
   throw new Error("Configure signal and control pins");

new device.io.PulseCount({
   signal,
   control,
   onReadable() {
      const count = this.read();
      trace(`count: ${count}\n`);
   }
});
