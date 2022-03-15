/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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

import {ThreeIntegers} from "exampleView";

function zero(view) @ "xs_zero";
function increment(view) @ "xs_increment";

const i3 = new ThreeIntegers;

i3.x = 1; 
i3.y = 2; 
i3.z = 3; 
trace(JSON.stringify(i3), "\n");

increment(i3);
trace(JSON.stringify(i3), "\n");

zero(i3);
trace(JSON.stringify(i3), "\n");
