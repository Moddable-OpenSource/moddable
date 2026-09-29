import {Outline} from "commodetto/outline";

const roundRect = {
	__proto__: Content.prototype,
	_create($, it) { native("PiuRoundRect_create").call(this, $, it); },
	place(width, height, corners, radius) {
		const topLeft = corners & RoundRect.topLeft ? radius : 0;
		const topRight = corners & RoundRect.topRight ? radius : 0;
		const bottomLeft = corners & RoundRect.bottomLeft ? radius : 0;
		const bottomRight = corners & RoundRect.bottomRight ? radius : 0;
		const path = Outline.RoundRectPath(0, 0, width, height, topLeft, topRight, bottomRight, bottomLeft);
		return Outline.fill(path, Outline.NON_ZERO_RULE);
	},
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