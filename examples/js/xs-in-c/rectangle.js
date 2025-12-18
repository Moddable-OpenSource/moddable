/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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

class Rectangle extends Native("xs_rectangle_destructor") {
	constructor(...params) { super(); native("xs_rectangle").call(this, ...params); }

	get x() { return native("xs_rectangle_get_x").call(this); }
	set x(it) { native("xs_rectangle_set_x").call(this, it); }
	get y() { return native("xs_rectangle_get_y").call(this); }
	set y(it) { native("xs_rectangle_set_y").call(this, it); }
	get w() { return native("xs_rectangle_get_w").call(this); }
	set w(it) { native("xs_rectangle_set_w").call(this, it); }
	get h() { return native("xs_rectangle_get_h").call(this); }
	set h(it) { native("xs_rectangle_set_h").call(this, it); }

	contains(x, y) { return native("xs_rectangle_contains").call(this, x, y); }
	union(r) { native("xs_rectangle_union").call(this, r); }
};

export default Rectangle;
