/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
 export class VerticalScrollerBehavior extends Behavior {
	onTouchBegan(scroller, id, x, y) {
		this.anchor = scroller.scroll.y;
		this.y = y;
		this.waiting = true;
	}
	onTouchMoved(scroller, id, x, y, ticks) {
		let delta = y - this.y;
		if (this.waiting) {
			if (Math.abs(delta) < 8)
				return;
			this.waiting = false;
			scroller.captureTouch(id, x, y, ticks);
		}
		scroller.scrollTo(0, this.anchor - delta);
	}
}

export class HorizontalScrollerBehavior extends Behavior {
	onTouchBegan(scroller, id, x, y) {
		this.anchor = scroller.scroll.x;
		this.x = x;
		this.waiting = true;
	}
	onTouchMoved(scroller, id, x, y, ticks) {
		let delta = x - this.x;
		if (this.waiting) {
			if (Math.abs(delta) < 8)
				return;
			this.waiting = false;
			scroller.captureTouch(id, x, y, ticks);
		}
		scroller.scrollTo(this.anchor - delta, 0);
	}
}