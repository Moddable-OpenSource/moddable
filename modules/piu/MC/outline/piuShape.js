import { Outline } from "commodetto/outline";

const shape = {
	__proto__: Content.prototype,
	_create($, it) { return native("PiuShape_create").call(this, $, it); },
	
	get fillOutline() { return native("PiuShape_get_fillOutline").call(this); },
	get strokeOutline() { return native("PiuShape_get_strokeOutline").call(this); },
	
	set fillOutline(it) { native("PiuShape_set_fillOutline").call(this, it); },
	set strokeOutline(it) { native("PiuShape_set_strokeOutline").call(this, it); },
};
export const Shape = Template(shape);
Shape.Outline = Outline;
Object.freeze(shape);
globalThis.Shape = Shape;
