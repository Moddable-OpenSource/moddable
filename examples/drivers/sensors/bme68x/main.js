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

import forced_mode from "examples/forced_mode"
import parallel_mode from "examples/parallel_mode"
import sequential_mode from "examples/sequential_mode"
import self_test from "examples/self_test"
import sensor from "examples/sensor"

/*
	Enable one of the imported example modules
	
	- sensor shows how to use the ECMA-419 sensor driver
	- forced_mode, parallel_mode, sequential_mode are JavaScript versions of the corresponding Bosch examples
	- self_test shows how to use the factory test mode
*/

sensor();
//forced_mode();
//sequential_mode();
//parallel_mode();
//self_test();
