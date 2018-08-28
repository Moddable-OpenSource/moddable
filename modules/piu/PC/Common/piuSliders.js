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

class SliderBehavior extends Behavior {
	changeState(container, state) {
		container.last.state = state;
	}
	getMax(container) {
		return this.data.max;
	}
	getMin(container) {
		return this.data.min;
	}
	getOffset(container, size) {
		var min = this.getMin(container);
		var max = this.getMax(container);
		var value = this.getValue(container);
		return Math.round(((value - min) * size) / (max - min));
	}
	getValue(container) {
		return this.data.value;
	}
	onAdapt(container) {
		this.onLayoutChanged(container);
	}
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
		this.onLayoutChanged(container);
	}
	onLayoutChanged(container) {
	}
	onTouchBegan(container, id, x, y, ticks) {
		container.captureTouch(id, x, y, ticks);
		this.changeState(container, 1);
		this.onTouchMoved(container, id, x, y, ticks);
	}
	onTouchEnded(container, id, x, y, ticks) {
		this.changeState(container, 0);
		this.onValueChanged(container);
	}
	onTouchMoved(container, id, x, y, ticks) {
		debugger
	}
	onValueChanged(container) {
	}
	onValueChanging(container) {
	}
	setOffset(container, size, offset) {
		var min = this.getMin(container);
		var max = this.getMax(container);
		var value = min + ((offset * (max - min)) / size);
		if (value < min) value = min;
		else if (value > max) value = max;
		this.setValue(container, value);
	}
	setValue(container, value) {
		var data = this.data;
		if ("step" in data)
			value = data.step * Math.round(value / data.step);
		data.value = value;
	}
};

export class HorizontalSliderBehavior extends SliderBehavior {
	onLayoutChanged(container) {
		var button = container.last;
		var bar = button.previous;
		var background = bar.previous;
		var size = (background.width - button.width);
		var offset = this.getOffset(container, size);
		button.x = background.x + offset;
		bar.width = button.width + offset;
	}
	onTouchMoved(container, id, x, y, ticks) {
		var button = container.last;
		var bar = button.previous;
		var background = bar.previous;
		var size = (background.width - button.width);
		var offset = (x - (button.width >> 1) - background.x);
		this.setOffset(container, size, offset);
		this.onLayoutChanged(container);
		this.onValueChanging(container);
	}
};

export class HorizontalLogSliderBehavior extends HorizontalSliderBehavior {
	getOffset(container, size) {
		var min = this.getMin(container);
		var max = this.getMax(container);
		var value = this.getValue(container);
		var logMin = Math.log(min);
		var maxv = Math.log(max);
		return Math.round(((Math.log(value) - logMin) * size) / (maxv - logMin));
	}
	setOffset(container, size, offset) {
		var min = this.getMin(container);
		var max = this.getMax(container);
		var logMin = Math.log(min);
		var logMax = Math.log(max);
		var value = Math.exp(logMin + (offset * (logMax - logMin) / size));
		if (value < min) value = min;
		else if (value > max) value = max;
		this.setValue(container, value);
	}
};

export class VerticalSliderBehavior extends SliderBehavior {
	onLayoutChanged(container) {
		var button = container.last;
		var bar = button.previous;
		var background = bar.previous;
		var size = (background.height - button.height);
		var offset = this.getOffset(container, size);
		button.y = background.y + background.height - offset - button.height;
		bar.height = button.height + offset;
	}
	onTouchMoved(container, id, x, y, ticks) {
		var button = container.last;
		var bar = button.previous;
		var background = bar.previous;
		var size = (background.height - button.height);
		var offset = background.y + background.height - (y + (button.height >> 1));
		this.setOffset(container, size, offset);
		this.onLayoutChanged(container);
		this.onValueChanging(container);
	}
};

export class VerticalLogSliderBehavior extends VerticalSliderBehavior {
	getOffset(container, size) {
		var min = this.getMin(container);
		var max = this.getMax(container);
		var value = this.getValue(container);
		var logMin = Math.log(min);
		var maxv = Math.log(max);
		return Math.round(((Math.log(value) - logMin) * size) / (maxv - logMin));
	}
	setOffset(container, size, offset) {
		var min = this.getMin(container);
		var max = this.getMax(container);
		var logMin = Math.log(min);
		var logMax = Math.log(max);
		var value = Math.exp(logMin + (offset * (logMax - logMin) / size));
		if (value < min) value = min;
		else if (value > max) value = max;
		this.setValue(container, value);
	}
};

// TEMPLATES

export var HorizontalSlider = Container.template(($, it) => ({
	active:true, Behavior:HorizontalSliderBehavior,
	contents:[
		Content($, { left:0, right:0, top:0, bottom:0, skin:it.barSkin, state:0 }),
		Content($, { left:0, width:0, top:0, bottom:0, skin:it.barSkin, state:1 }),
		Content($, { left:0, top:0, bottom:0, skin:it.buttonSkin, state:0 }),
	]
}));

export var HorizontalLogSlider = Container.template(($, it) => ({
	active:true, Behavior:HorizontalLogSliderBehavior,
	contents:[
		Content($, { left:0, right:0, top:0, bottom:0, skin:it.barSkin, state:0 }),
		Content($, { left:0, width:0, top:0, bottom:0, skin:it.barSkin, state:1 }),
		Content($, { left:0, top:0, bottom:0, skin:it.buttonSkin, state:0 }),
	]
}));

export var VerticalSlider = Container.template(($, it) => ({
	active:true, Behavior:VerticalSliderBehavior,
	contents:[
		Content($, { left:0, right:0, top:0, bottom:0, skin:it.barSkin, state:0 }),
		Content($, { left:0, right:0, height:0, bottom:0, skin:it.barSkin, state:1 }),
		Content($, { left:0, right:0, top:0, skin:it.buttonSkin, state:0 }),
	]
}));

export var VerticalLogSlider = Container.template(($, it) => ({
	active:true, Behavior:VerticalLogSliderBehavior,
	contents:[
		Content($, { left:0, right:0, top:0, bottom:0, skin:it.barSkin, state:0 }),
		Content($, { left:0, right:0, height:0, bottom:0, skin:it.barSkin, state:1 }),
		Content($, { left:0, right:0, top:0, skin:it.buttonSkin, state:0 }),
	]
}));
