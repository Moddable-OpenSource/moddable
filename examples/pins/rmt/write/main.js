/*
 * Copyright (c) 2021 Moddable Tech, Inc.
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

import RMT from "pins/rmt";

let rmt = new RMT({pin: 17, channel: 1, divider: 100});

rmt.onWritable = function() {
	rmt.write(1, [2000, 5000, 2000, 5000, 2000, 5000, 10000]);
}

