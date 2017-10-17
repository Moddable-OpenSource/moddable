/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
	ArduCAM Mini driver

	2 Megapixel - OV2640 controller chip
*/

export default class ArduCAM @ "xs_arducam_destructor" {
	constructor(dictionary) @ "xs_arducam";
	capture() @ "xs_arducam_capture";
	read(buffer) @  "xs_arducam_read";
	close() @ "xs_arducam_close";
}

/*

	dictionary:
	
		{width: 320, height: 240, format: "rgb565be"}
		{width: 320, height: 240, format: "rgb565le"}
		{width: 160, height: 120, format: "jpeg"}
*/

