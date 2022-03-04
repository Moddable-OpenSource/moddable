import { Outline } from "commodetto/outline";

const shape = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuShape_create",
	
	get fillOutline() @ "PiuShape_get_fillOutline",
	get strokeOutline() @ "PiuShape_get_strokeOutline",
	
	set fillOutline(it) @ "PiuShape_set_fillOutline",
	set strokeOutline(it) @ "PiuShape_set_strokeOutline",
};
export const Shape = Template(shape);
Shape.Outline = Outline;
Object.freeze(shape);
globalThis.Shape = Shape;
