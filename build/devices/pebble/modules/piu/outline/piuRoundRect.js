const roundRect = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuRoundRect_create",
	get corners() @ "PiuRoundRect_get_corners",
	set corners(it) @ "PiuRoundRect_set_corners",
	get radius() @ "PiuRoundRect_get_radius",
	set radius(it) @ "PiuRoundRect_set_radius",
};
export const RoundRect = Template(roundRect);
RoundRect.topLeft = 1;
RoundRect.topRight = 2;
RoundRect.bottomLeft = 4;
RoundRect.bottomRight = 8;
Object.freeze(roundRect);
globalThis.RoundRect = RoundRect;
export default RoundRect;