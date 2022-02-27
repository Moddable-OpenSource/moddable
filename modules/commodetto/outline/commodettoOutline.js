class Outline @ "xs_outline_destructor" {
	constructor() @ "xs_outline"
	
	clone() @ "xs_outline_clone"
	rotate(angle, cx, cy) @ "xs_outline_rotate"
	scale(x, y) @ "xs_outline_scale"
	translate(x, y) @ "xs_outline_translate"
	
	get bounds() @ "xs_outline_get_bounds"
	
	static fill(path, rule) @ "xs_outline_fill"
	static stroke(path, weight, linecap, linejoin, miterLimit) @ "xs_outline_stroke"
	
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
	beginSubpath(x, y, open) @ "xs_outline_FreeTypePath_beginSubpath"
	conicTo(cx, cy, x, y) @ "xs_outline_FreeTypePath_conicTo"
	cubicTo(c1x, c1y, c2x, c2y, x, y) @ "xs_outline_FreeTypePath_cubicTo"
	endSubpath() @ "xs_outline_FreeTypePath_endSubpath"
	lineTo(x, y) @ "xs_outline_FreeTypePath_lineTo"
}

Outline.PolygonPath = function(x0, y0, x1, y1 /* etc */) @ "xs_outline_PolygonPath"
Outline.RoundRectPath = function(x, y, w, h, r) @ "xs_outline_RoundRectPath"

Outline.CanvasPath = class extends Array {
	arc(x, y, radius, startAngle, endAngle, counterclockwise) @ "xs_outline_CanvasPath_arc"
	arcTo(x1, y1, x2, y2, r) @ "xs_outline_CanvasPath_arcTo"
	bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y) @ "xs_outline_CanvasPath_bezierCurveTo"
	closePath() @ "xs_outline_CanvasPath_closePath"
	ellipse(x, y, radiusX, radiusY, rotation, startAngle, endAngle, counterclockwise) @ "xs_outline_CanvasPath_ellipse"
	lineTo(x, y) @ "xs_outline_CanvasPath_lineTo"
	moveTo(x, y) @ "xs_outline_CanvasPath_moveTo"
	quadraticCurveTo(cpx, cpy, x, y) @ "xs_outline_CanvasPath_quadraticCurveTo"
	rect(x, y, w, h) @ "xs_outline_CanvasPath_rect"
}

Outline.SVGPath = function(path) @ "xs_outline_SVGPath"

export { Outline }