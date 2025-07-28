const svgImage = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuSVGImage_create",
	
	get opacity() @ "PiuSVGImage_get_opacity",
	set opacity(it) @ "PiuSVGImage_set_opacity",
	
	center(x, y) @ "PiuSVGImage_center",
	get cx() @ "PiuSVGImage_get_cx",
	set cx(it) @ "PiuSVGImage_set_cx",
	get cy() @ "PiuSVGImage_get_cy",
	set cy(it) @ "PiuSVGImage_set_cy",
	
	rotate(a) @ "PiuSVGImage_rotate",
	get r() @ "PiuSVGImage_get_r",
	set r(it) @ "PiuSVGImage_set_r",

	scale(x, y) @ "PiuSVGImage_scale",
	get s(it) @ "PiuSVGImage_get_s",
	set s(it) @ "PiuSVGImage_set_s",
	get sx() @ "PiuSVGImage_get_sx",
	set sx(it) @ "PiuSVGImage_set_sx",
	get sy() @ "PiuSVGImage_get_sy",
	set sy(it) @ "PiuSVGImage_set_sy",
	
	translate(x, y) @ "PiuSVGImage_translate",
	get tx() @ "PiuSVGImage_get_tx",
	set tx(it) @ "PiuSVGImage_set_tx",
	get ty() @ "PiuSVGImage_get_ty",
	set ty(it) @ "PiuSVGImage_set_ty",
};
export const SVGImage = Template(svgImage);
Object.freeze(svgImage);
globalThis.SVGImage = SVGImage;
export default SVGImage;