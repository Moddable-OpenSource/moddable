/*
 *   Copyright (c) 2020 Shinya Ishikawa
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
export class HX711 @ "xs_HX711_destructor" {
	constructor(dictionary) @ "xs_HX711";

	/* accessor */
	get offset() @ "xs_HX711_get_offset"
	set offset(offset) @ "xs_HX711_set_offset"
	get scale() @ "xs_HX711_get_scale"
	set scale(scale) @ "xs_HX711_set_scale"

	// readonly
	get value() @ "xs_HX711_get_value";
	get rawValue() @ "xs_HX711_get_raw_value"
	get clk() @ "xs_HX711_get_clk_pin"
	get dat() @ "xs_HX711_get_dat_pin"
	resetOffset() {
		this.offset = this.rawValue
	}
};

export default HX711;
