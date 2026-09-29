import {Outline} from "commodetto/outline";

function shapesToCommands(shapes) {
	const commands = [];
	const clone = null;
	for (let $ of shapes) {
		if ($.fill) {
			const outline = Outline.fill($.path, $.fill.rule);
			const color = rgba($.fill.color.r, $.fill.color.g, $.fill.color.b, $.fill.color.a);
			commands.push({ outline, color, clone });
		}
		if ($.stroke) {
			const outline = Outline.stroke($.path, $.stroke.weight, $.stroke.linecap, $.stroke.linejoin);
			const color = rgba($.stroke.color.r, $.stroke.color.g, $.stroke.color.b, $.stroke.color.a);
			commands.push({ outline, color, clone });
		}
	}
	return commands;
}

const svgImage = {
	__proto__: Content.prototype,
	_create($, it) { native("PiuSVGImage_create").call(this, $, it); },
	load(resource) {
		const result = {
			width: resource.width,
			height: resource.height,
		};
		let duration  = 0;
		if (resource.frames) {
			result.frames = resource.frames.map(frame => {
				let time = duration;
				duration += frame.duration;
				return {
					commands: shapesToCommands(frame.shapes),
					time
				};
			});
			
		}
		else {
			result.frames = [
				{ 
					commands: shapesToCommands(resource.shapes),
					time: 0,
				} 
			];
		}
		this.duration = duration;
		return result;
	},
	unload(resource) {
	},
	
	get opacity() { return native("PiuSVGImage_get_opacity").call(this); },
	set opacity(it) { native("PiuSVGImage_set_opacity").call(this, it); },
	
	center(x, y) { return native("PiuSVGImage_center").call(this, x, y); },
	get cx() { return native("PiuSVGImage_get_cx").call(this); },
	set cx(it) { native("PiuSVGImage_set_cx").call(this, it); },
	get cy() { return native("PiuSVGImage_get_cy").call(this); },
	set cy(it) { native("PiuSVGImage_set_cy").call(this, it); },
	
	rotate(a) { return native("PiuSVGImage_rotate").call(this, a); },
	get r() { return native("PiuSVGImage_get_r").call(this); },
	set r(it) { native("PiuSVGImage_set_r").call(this, it); },

	scale(x, y) { return native("PiuSVGImage_scale").call(this, x, y); },
	get s() { return native("PiuSVGImage_get_s").call(this); },
	set s(it) { native("PiuSVGImage_set_s").call(this, it); },
	get sx() { return native("PiuSVGImage_get_sx").call(this); },
	set sx(it) { native("PiuSVGImage_set_sx").call(this, it); },
	get sy() { return native("PiuSVGImage_get_sy").call(this); },
	set sy(it) { native("PiuSVGImage_set_sy").call(this, it); },
	
	translate(x, y) { return native("PiuSVGImage_translate").call(this, x, y); },
	get tx() { return native("PiuSVGImage_get_tx").call(this); },
	set tx(it) { native("PiuSVGImage_set_tx").call(this, it); },
	get ty() { return native("PiuSVGImage_get_ty").call(this); },
	set ty(it) { native("PiuSVGImage_set_ty").call(this, it); },
};
export const SVGImage = Template(svgImage);
Object.freeze(svgImage);
globalThis.SVGImage = SVGImage;
export default SVGImage;