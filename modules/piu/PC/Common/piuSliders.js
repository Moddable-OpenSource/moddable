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

export class ProgressBarBehavior extends Behavior {
	getMax(container) {
		return this.data.max;
	}
	getMin(container) {
		return this.data.min;
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
	onDataChanged(container) {
		let active = container.active;
		var bar = container.first;
		var progress = bar.first;
		bar.state = active ? 1 : 0;
		progress.state = active ? 3 : 0;
		this.onLayoutChanged(container);
	}
	onDisplaying(container) {
		this.onDataChanged(container);
	}
	onLayoutChanged(container) {
		var bar = container.first;
		var progress = bar.first;
		var min = this.getMin(container);
		var max = this.getMax(container);
		var value = this.getValue(container);
		progress.width = Math.round(((value - min) * bar.width) / (max - min));
	}
}

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
	onDataChanged(container) {
		let active = container.active;
		var button = container.last;
		var bar = button.previous;
		var background = bar.previous;
		background.state = active ? 1 : 0;
		bar.state = active ? 2 : 0;
		button.state = active ? 1 : 0;
		this.onLayoutChanged(container);
	}
	onDisplaying(container) {
		var button = container.last;
		this.dx = button.x - container.x;
		this.dy = button.y - container.y;
		let data = this.data;
		this.onDataChanged(container);
	}
	onLayoutChanged(container) {
	}
	onTouchBegan(container, id, x, y, ticks) {
		container.captureTouch(id, x, y, ticks);
		this.onTouchMoved(container, id, x, y, ticks);
	}
	onTouchEnded(container, id, x, y, ticks) {
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
		var size = (container.width - button.width - (this.dx << 1));
		var offset = this.getOffset(container, size);
		var x = button.x = container.x + this.dx + offset;
		bar.width = x - background.x + (button.width >> 1);
	}
	onTouchMoved(container, id, x, y, ticks) {
		var button = container.last;
		var bar = button.previous;
		var background = bar.previous;
		var size = (container.width - button.width - (this.dx << 1));
		var offset = (x - (button.width >> 1) - container.x - this.dx);
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
		var size = (container.height - button.height - (this.dy << 1));
		var offset = this.getOffset(container, size);
		var y = button.y = container.y + this.dy + offset;
		bar.height = y - background.y + (button.height >> 1);
	}
	onTouchMoved(container, id, x, y, ticks) {
		var button = container.last;
		var bar = button.previous;
		var background = bar.previous;
		var size = (container.height - button.height - (this.dy << 1));
		var offset = (y - (button.height >> 1) - container.y - this.dy);
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

export var ProgressBar = Container.template(($, it) => ({
	height:30, active:true, Behavior:ProgressBarBehavior,
	contents:[
		Container($, { 
			left:5, right:5, height:10, skin:skins.progressBar, state:1,
			contents: [
				Content($, { left:0, width:0, top:0, bottom:0, skin:skins.progressBar, state:3 }),
			],
		}),
	]
}));

export var HorizontalSlider = Container.template(($, it) => ({
	height:30, active:true, Behavior:HorizontalSliderBehavior,
	contents:[
		RoundContent($, { left:11, right:11, top:11, height:8, border:1, radius:4, skin:skins.sliderBar }),
		RoundContent($, { left:11, width:0, top:11, height:8, border:1, radius:4, skin:skins.sliderBar }),
		RoundContent($, { left:6, width:18, top:6, height:18, border:1, radius:9, skin:skins.sliderButton }),
	]
}));

export var HorizontalLogSlider = Container.template(($, it) => ({
	height:30, active:true, Behavior:HorizontalLogSliderBehavior,
	contents:[
		RoundContent($, { left:11, right:11, top:11, height:8, border:1, radius:4, skin:skins.sliderBar }),
		RoundContent($, { left:11, width:0, top:11, height:8, border:1, radius:4, skin:skins.sliderBar }),
		RoundContent($, { left:6, width:18, top:6, height:18, border:1, radius:9, skin:skins.sliderButton }),
	]
}));

export var VerticalSlider = Container.template(($, it) => ({
	width:30, active:true, Behavior:VerticalSliderBehavior,
	contents:[
		RoundContent($, { left:11, width:8, top:11, bottom:11, border:1, radius:4, skin:skins.sliderBar }),
		RoundContent($, { left:11, width:8, height:0, bottom:11, border:1, radius:4, skin:skins.sliderBar }),
		RoundContent($, { left:6, width:18, top:6, height:18, border:1, radius:9, skin:skins.sliderButton }),
	]
}));

export var VerticalLogSlider = Container.template(($, it) => ({
	width:30, active:true, Behavior:VerticalLogSliderBehavior,
	contents:[
		RoundContent($, { left:11, width:8, top:11, bottom:11, border:1, radius:4, skin:skins.sliderBar }),
		RoundContent($, { left:11, width:8, height:0, bottom:11, border:1, radius:4, skin:skins.sliderBar }),
		RoundContent($, { left:6, width:18, top:6, height:18, border:1, radius:9, skin:skins.sliderButton }),
	]
}));
