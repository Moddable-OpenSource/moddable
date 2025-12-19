const roundRect = {
	__proto__: Content.prototype,
	_create($, it) { return native("PiuRoundRect_create").call(this, $, it); },
	get corners() { return native("PiuRoundRect_get_corners").call(this); },
	set corners(it) { native("PiuRoundRect_set_corners").call(this, it); },
	get radius() { return native("PiuRoundRect_get_radius").call(this); },
	set radius(it) { native("PiuRoundRect_set_radius").call(this, it); },
};
export const RoundRect = Template(roundRect);
RoundRect.topLeft = 1;
RoundRect.topRight = 2;
RoundRect.bottomLeft = 4;
RoundRect.bottomRight = 8;
Object.freeze(roundRect);
globalThis.RoundRect = RoundRect;
export default RoundRect;