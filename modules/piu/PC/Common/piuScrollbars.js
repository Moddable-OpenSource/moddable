/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
	
// BEHAVIORS

export class ScrollerBehavior extends Behavior {
	onMouseScrolled(scroller, dx, dy) {
		if (Math.abs(dx) > Math.abs(dy))
			scroller.scrollBy(-dx, 0);
		else
			scroller.scrollBy(0, -dy);
	}
};

class ScrollbarBehavior extends Behavior {
	onCreate(scrollbar) {
		scrollbar.duration = 2000;
		this.former = 0;
	}
	onFinished(scrollbar) {
		scrollbar.first.state = 0;
	}
	onMouseEntered(scrollbar, x, y) {
		application.cursor = cursors.arrow;
		var thumb = scrollbar.first;
		scrollbar.state = 1;
		thumb.state = 2;
		scrollbar.stop();
	}
	onMouseExited(scrollbar, x, y) {
		var thumb = scrollbar.first;
		scrollbar.state = 0;
		thumb.state = 1;
		scrollbar.time = 0;
		scrollbar.start();
	}
	onScrolled(scrollbar, scroller = scrollbar.container) {
		let current = scroller.scroll[this.direction];
		if (this.former != current) {
			this.former = current;
			var thumb = scrollbar.first;
			if (thumb.state <= 1) {
				thumb.state = 1;
				scrollbar.time = 0;
				scrollbar.start();
			}
		}
	}
	onTimeChanged(scrollbar) {
		let fraction = scrollbar.fraction;
		let state = (fraction > 0.5) ? 1 - Math.quadEaseOut(2 * (fraction - 0.5)) : 1;
		scrollbar.first.state = state;
	}
};

class HorizontalScrollbarBehavior extends ScrollbarBehavior {
	onCreate(scrollbar) {
		super.onCreate(scrollbar);
		this.direction = "x";
	}
	onScrolled(scrollbar, scroller = scrollbar.container) {
		super.onScrolled(scrollbar, scroller);
		var thumb = scrollbar.first;
		var size = scroller.width;
		var range = scroller.first.width;
		if (size < range) {
			var width = scrollbar.width;
			thumb.x = scrollbar.x + Math.round(scroller.scroll.x * width / range);
			thumb.width = Math.round(width * size / range);
			scrollbar.visible = scrollbar.active = true;
		}
		else {
			thumb.width = 0;
			scrollbar.visible = scrollbar.active = false;
		}
	}
	onTouchBegan(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 3;
		scrollbar.captureTouch(id, x, y, ticks);
		let thumbY = thumb.x;
		let thumbHeight = thumb.width;
		if ((x < thumbY) || ((thumbY + thumbHeight) <= x))
			this.anchor = 0 - (thumbHeight >> 1);
		else
			this.anchor = thumbY - x;
		this.min = scrollbar.x;
		this.max = scrollbar.x + scrollbar.width - thumbHeight;
		this.onTouchMoved(scrollbar, id, x, y, ticks);
	}
	onTouchEnded(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 2;
	}
	onTouchMoved(scrollbar, id, x, y, ticks) {
		var scroller = scrollbar.container;
		var thumb = scrollbar.first;
		x += this.anchor;
		if (x < this.min)
			x = this.min;
		else if (x > this.max)
			x = this.max;
		thumb.x = x;

		scroller.scrollTo((x - this.min) * (scroller.first.width / scrollbar.width), scroller.scroll.y);
	}
};

class VerticalScrollbarBehavior extends ScrollbarBehavior {
	onCreate(scrollbar) {
		super.onCreate(scrollbar);
		this.direction = "y";
	}
	onScrolled(scrollbar, scroller = scrollbar.container) {
		super.onScrolled(scrollbar, scroller);
		var thumb = scrollbar.first;
		var size = scroller.height;
		var range = scroller.first.height;
		if (size < range) {
			var height = scrollbar.height;
			thumb.y = scrollbar.y + Math.round(scroller.scroll.y * height / range);
			thumb.height = Math.round(height * size / range);
			scrollbar.visible = scrollbar.active = true;
		}
		else {
			thumb.height = 0;
			scrollbar.visible = scrollbar.active = false;
		}
	}
	onTouchBegan(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 3;
		scrollbar.captureTouch(id, x, y, ticks);
		let thumbY = thumb.y;
		let thumbHeight = thumb.height;
		if ((y < thumbY) || ((thumbY + thumbHeight) <= y))
			this.anchor = 0 - (thumbHeight >> 1);
		else
			this.anchor = thumbY - y;
		this.min = scrollbar.y;
		this.max = scrollbar.y + scrollbar.height - thumbHeight;
		this.onTouchMoved(scrollbar, id, x, y, ticks);
	}
	onTouchEnded(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 2;
	}
	onTouchMoved(scrollbar, id, x, y, ticks) {
		var scroller = scrollbar.container;
		var thumb = scrollbar.first;
		y += this.anchor;
		if (y < this.min)
			y = this.min;
		else if (y > this.max)
			y = this.max;
		thumb.y = y;

		scroller.scrollTo(scroller.scroll.x, (y - this.min) * (scroller.first.height / scrollbar.height));
	}
};

// TEMPLATES

export var HorizontalScrollbar = Container.template($ => ({
	left:0, right:0, height:10, bottom:0, skin:skins.horizontalScrollbar, active:true, Behavior:HorizontalScrollbarBehavior,
	contents: [
		Content($, { left:0, width:0, height:9, bottom:0, skin:skins.scrollbarThumb }),
	],
}));

export var VerticalScrollbar = Container.template($ => ({
	width:10, right:0, top:0, bottom:0, skin:skins.verticalScrollbar, active:true, Behavior:VerticalScrollbarBehavior,
	contents: [
		Content($, { right:0, width:9, top:0, height:0, skin:skins.scrollbarThumb }),
	],
}));
