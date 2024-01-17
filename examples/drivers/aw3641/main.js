/*
 * Copyright (c) 2021  Moddable Tech, Inc.
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

import Timer from "timer";
import AW3641 from "aw3641";

const lamp = new AW3641({pin:26,});

Timer.repeat(() => {
	lamp.flash(AW3641.Time220ms_Brightness50);
	trace("flash\n");
}, 2000);


