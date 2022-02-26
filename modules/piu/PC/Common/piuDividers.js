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

export class DividerLayoutBehavior extends Behavior {
	onAdapt(layout) {
		var divider = layout.last;
		divider.behavior.divide(divider);
	}
	onDisplaying(layout) {
		var divider = layout.last;
		divider.behavior.divide(divider);
	}
};

class DividerBehavior extends Behavior {
	onCreate(divider, data, dictionary) {
		this.after = ("after" in dictionary) ? dictionary.after : 0;
		this.before = ("before" in dictionary) ? dictionary.before : 0;
		this.current = ("current" in dictionary) ? dictionary.current : 0;
		this.status = ("status" in dictionary) ? dictionary.status : false;
	}
	onTouchEnded(divider, id, x, y, ticks) {
		var container = divider.container;
		var current = this.measure(divider);
		this.status = (this.direction > 0) ? this.before != current : this.after != current;
		if (this.status)
			this.current = current;
		application.distribute("onDividerChanged", divider);
	}
};

export class HorizontalDividerBehavior extends DividerBehavior {
	divide(divider) {
		var layout = divider.container;
		var height = divider.height >> 1;
		var top = divider.y + height - layout.y;
		var bottom = layout.height - top;
		if (top < this.before) {
			top = this.before;
			bottom = layout.height - top;
		}
		if (bottom < this.after) {
			bottom = this.after;
			top = layout.height - bottom;
		}
		divider.y = layout.y + top - height;
		divider.previous.previous.height = top;
		divider.previous.height = bottom;
	}
	measure(divider) {
		var layout = divider.container;
		var height = divider.height >> 1;
		if (this.direction > 0)
			return divider.y + height - layout.y;
		return (layout.y + layout.height) - (divider.y + height);
	}
	onCreate(divider, data, dictionary) {
		super.onCreate(divider, data, dictionary);
		this.direction = ("top" in dictionary) ? 1 : -1;
	}
	onMouseEntered(divider, x, y) {
		application.cursor = cursors.resizeRow;
	}
	onTouchBegan(divider, id, x, y, ticks) {
		this.anchor = y - divider.y;
		this.status = true;
	}
	onTouchMoved(divider, id, x, y, ticks) {
		divider.y = y - this.anchor;
		this.divide(divider);
	}
	toggle(divider) {
		var layout = divider.container;
		var height = divider.height >> 1;
		if (this.status) {
			this.status = false;
			if (this.direction > 0)
				divider.y = layout.y + this.before - height;
			else
				divider.y = (layout.y + layout.height) - (this.after + height);
		}
		else {
			this.status = true;
			if (this.direction > 0)
				divider.y = layout.y + this.current - height;
			else
				divider.y = (layout.y + layout.height) - (this.current + height);
		}
		this.divide(divider);
		application.distribute("onDividerChanged", divider);
	}
};

export class VerticalDividerBehavior extends DividerBehavior {
	divide(divider) {
		var layout = divider.container;
		var width = divider.width >> 1;
		var left = divider.x + width - layout.x;
		var right = layout.width - left;
		if (left < this.before) {
			left = this.before;
			right = layout.width - left;
		}
		if (right < this.after) {
			right = this.after;
			left = layout.width - right;
		}
		divider.x = layout.x + left - width;
		divider.previous.previous.width = left;
		divider.previous.width = right;
	}
	measure(divider) {
		var layout = divider.container;
		var width = divider.width >> 1;
		if (this.direction > 0)
			return divider.x + width - layout.x;
		return (layout.x + layout.width) - (divider.x + width);
	}
	onCreate(divider, data, dictionary) {
		super.onCreate(divider, data, dictionary);
		this.direction = ("left" in dictionary) ? 1 : -1;
	}
	onMouseEntered(divider, x, y) {
		application.cursor = cursors.resizeColumn;
	}
	onTouchBegan(divider, id, x, y, ticks) {
		this.anchor = x - divider.x;
	}
	onTouchMoved(divider, id, x, y, ticks) {
		divider.x = x - this.anchor;
		this.divide(divider);
	}
	toggle(divider) {
		var layout = divider.container;
		var width = divider.width >> 1;
		if (this.status) {
			this.status = false;
			if (this.direction > 0)
				divider.x = layout.x + this.before - width;
			else
				divider.x = (layout.x + layout.width) - (this.after + width);
		}
		else {
			this.status = true;
			if (this.direction > 0)
				divider.x = layout.x + this.current - width;
			else
				divider.x = (layout.x + layout.width) - (this.current + width);
		}
		this.divide(divider);
		application.distribute("onDividerChanged", divider);
	}
};

// TEMPLATES

export var HorizontalDivider = Container.template($ => ({
	left:0, right:0, height:6, active:true, Behavior:HorizontalDividerBehavior,
	 contents: [
	 	Content($, { left:0, right:0, height:1, bottom:3, skin:skins.divider }),
	 ],
}));

export var VerticalDivider = Container.template($ => ({
	 width:6, top:0, bottom:0, active:true, Behavior:VerticalDividerBehavior,
	 contents: [
	 	Content($, { width:1, right:3, top:0, bottom:0, skin:skins.divider }),
	 ],
 }));

