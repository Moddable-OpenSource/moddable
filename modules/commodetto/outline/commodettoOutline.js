/*
 * Copyright (c) 2019-2025  Moddable Tech, Inc.
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

class Outline extends Native("xs_outline_destructor") {
	constructor() { super(); native("xs_outline").call(this); }

	clone() { return native("xs_outline_clone").call(this); }
	rotate(angle, cx, cy) { return native("xs_outline_rotate").call(this, angle, cx, cy); }
	scale(x, y) { return native("xs_outline_scale").call(this, x, y); }
	translate(x, y) { return native("xs_outline_translate").call(this, x, y); }
	transform(cx, cy, a, sx, sy, tx, ty) { return native("xs_outline_transform").call(this, cx, cy, a, sx, sy, tx, ty); }

	get bounds() { return native("xs_outline_get_bounds").call(this); }

	static fill(path, rule) { return native("xs_outline_fill").call(this, path, rule); }
	static stroke(path, weight, linecap, linejoin, miterLimit) { return native("xs_outline_stroke").call(this, path, weight, linecap, linejoin, miterLimit); }

	static NON_ZERO_RULE = 0;
	static EVEN_ODD_RULE = 2;

	static LINECAP_BUTT = 0;
	static LINECAP_ROUND = 1;
	static LINECAP_SQUARE = 2;

	static LINEJOIN_ROUND = 0;
	static LINEJOIN_BEVEL = 1;
	static LINEJOIN_MITER = 2;
}
Outline.FreeTypePath = class extends Array {
	beginSubpath(x, y, open) { return native("xs_outline_FreeTypePath_beginSubpath").call(this, x, y, open); }
	conicTo(cx, cy, x, y) { return native("xs_outline_FreeTypePath_conicTo").call(this, cx, cy, x, y); }
	cubicTo(c1x, c1y, c2x, c2y, x, y) { return native("xs_outline_FreeTypePath_cubicTo").call(this, c1x, c1y, c2x, c2y, x, y); }
	endSubpath() { return native("xs_outline_FreeTypePath_endSubpath").call(this); }
	lineTo(x, y) { return native("xs_outline_FreeTypePath_lineTo").call(this, x, y); }
}

Outline.PolygonPath = function(x0, y0, x1, y1 /* etc */) { return native("xs_outline_PolygonPath").call(this, x0, y0, x1, y1); }
Outline.RoundRectPath = function(x, y, w, h, r) { return native("xs_outline_RoundRectPath").call(this, x, y, w, h, r); }

Outline.CanvasPath = class extends Array {
	arc(x, y, radius, startAngle, endAngle, counterclockwise) { return native("xs_outline_CanvasPath_arc").call(this, x, y, radius, startAngle, endAngle, counterclockwise); }
	arcTo(x1, y1, x2, y2, r) { return native("xs_outline_CanvasPath_arcTo").call(this, x1, y1, x2, y2, r); }
	bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y) { return native("xs_outline_CanvasPath_bezierCurveTo").call(this, cp1x, cp1y, cp2x, cp2y, x, y); }
	closePath() { return native("xs_outline_CanvasPath_closePath").call(this); }
	ellipse(x, y, radiusX, radiusY, rotation, startAngle, endAngle, counterclockwise) { return native("xs_outline_CanvasPath_ellipse").call(this, x, y, radiusX, radiusY, rotation, startAngle, endAngle, counterclockwise); }
	lineTo(x, y) { return native("xs_outline_CanvasPath_lineTo").call(this, x, y); }
	moveTo(x, y) { return native("xs_outline_CanvasPath_moveTo").call(this, x, y); }
	quadraticCurveTo(cpx, cpy, x, y) { return native("xs_outline_CanvasPath_quadraticCurveTo").call(this, cpx, cpy, x, y); }
	rect(x, y, w, h) { return native("xs_outline_CanvasPath_rect").call(this, x, y, w, h); }
}

Outline.SVGPath = function(path) { return native("xs_outline_SVGPath").call(this, path); }

export { Outline }
