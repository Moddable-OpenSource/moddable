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

export class SwitchBehavior extends Behavior {
	changeOffset(container, offset) {
		var bar = container.first;
		var button = bar.next;
		if (offset < 0)
			offset = 0;
		else if (offset > this.size)
			offset = this.size;
		else
			offset = Math.round(offset);
		this.offset = offset;
		bar.state = this.offset / this.size;
		button.x = container.x + this.offset;
	}
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
		var bar = container.first;
		var button = bar.next;
		this.size = bar.width - button.width;
		this.changeOffset(container, this.data.value ? this.size : 0);
	}
	onTimeChanged(container) {
		this.changeOffset(container, this.anchor + Math.round(this.delta * container.fraction));
	}
	onTouchBegan(container, id, x, y, ticks) {
		if (container.running) {
			container.stop();
			container.time = container.duration;
		}
		this.anchor = x;
		this.moved = false;
		this.delta = this.offset;
		container.captureTouch(id, x, y, ticks);
	}
	onTouchEnded(container, id, x, y, ticks) {
		var offset = this.offset;
		var size =  this.size;
		var delta = size >> 1;
		if (this.moved) {
			if (offset < delta)
				delta = 0 - offset;
			else 
				delta = size - offset;
		}
		else {
			if (offset == 0)
				delta = size;
			else if (offset == size)
				delta = 0 - size;
			else if (x > (container.x + (container.width >> 1)))
				delta = size - offset;
			else
				delta = 0 - offset;
		}
		if (delta) {
			this.anchor = offset;
			this.delta = delta;
			container.duration = 125 * Math.abs(delta) / size;
			container.time = 0;
			container.start();
		}
		var value = ((this.offset + delta) == 0) ? false : true;
		let data = this.data;
		if (data.value != value) {
			data.value = value;
			container.next.string = value ? data.on : data.off;
			this.onValueChanged(container);
		}
	}
	onTouchMoved(container, id, x, y, ticks) {
		this.moved = Math.abs(x - this.anchor) >= 8;
		this.changeOffset(container, this.delta + x - this.anchor);
	}
	onValueChanged(container) {
	}
};

// TEMPLATES

export var Switch = Container.template(($, it) => ({
	width:50, height:30, active:true,
	Behavior: SwitchBehavior,
	contents: [
		Content($, { left:0, right:0, height:30, skin:it.barSkin }),
		Content($, { left:0, width:30, height:30, skin:it.buttonSkin }),
	],
}));

