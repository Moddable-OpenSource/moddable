/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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

/*
 * Get a continuous GPS reading from SIM7100
 *   30 seconds between readings
 */

import Timer from "timer";
import SIM7100 from "sim7100";

const GPS_INTERVAL = 30;

let sim7100 = new SIM7100();

sim7100.onReady = function() { sim7100.enableGPS(); }
sim7100.onGPSEnabled = function(device) { device.getGPS(GPS_INTERVAL); }

sim7100.onGPSChanged = function(gps) {
	trace(`${gps.seq} GPS: lat:${gps.lat} lon:${gps.lon}\n`);
	trace(`  ${gps.date.month}/${gps.date.day}/${gps.date.year} @ ${gps.time.hour}:${gps.time.min}:${gps.time.sec}\n`);
	trace(`  Alt: ${gps.alt} m  -- Speed: ${gps.speed} -- Course: ${gps.course}˚\n`);
}

sim7100.start();

