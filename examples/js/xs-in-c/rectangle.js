/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

class Rectangle @ "xs_rectangle_destructor" {
	constructor(...params) @ "xs_rectangle";

	get x() @ "xs_rectangle_get_x";
	set x() @ "xs_rectangle_set_x";
	get y() @ "xs_rectangle_get_y";
	set y() @ "xs_rectangle_set_y";
	get w() @ "xs_rectangle_get_w";
	set w() @ "xs_rectangle_set_w";
	get h() @ "xs_rectangle_get_h";
	set h() @ "xs_rectangle_set_h";

	contains(x, y) @ "xs_rectangle_contains";

	union(r) @ "xs_rectangle_union";
};

export default Rectangle;
