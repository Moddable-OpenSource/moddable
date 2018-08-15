/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 *
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// ASSETS

import {
	buttonSkin,
	buttonStyle,
	controlNameStyle,
	controlValueStyle,
	controlsMenuSkin,
	controlsMenuGlyphSkin,
	controlsMenuItemSkin,
	controlsMenuItemStyle,
	controlRowSkin,
	dotSkin,
	glyphsSkin,
	paneBodySkin,
	paneBorderSkin,
	paneHeaderSkin,
	paneHeaderStyle,
	popupStyle,
	sliderBarSkin,
	sliderButtonSkin,
	switchBarSkin,
	switchButtonSkin,
	timerSkin,
} from "assets";	

// BEHAVIORS

import {
	ButtonBehavior,
} from "piu/Buttons";

class ControlsHeaderBehavior extends Behavior {	
	onDeviceSelected(row, device) {
		row.active = false;
		row.first.state = 0;
		row.last.state = 0;
		row.last.string = device.title;
		if (model.devices.length > 0) {
			if (model.devices.length > 1) {
				row.active = true;
				row.first.state = 1;
			}
			row.last.state = 1;
		}
	}
	onMenuSelected(row, index) {
		if (index >= 0)
			model.onSelectDevice(application, index);
	}
	onMouseEntered(row, x, y) {
		row.state = 1;
	}
	onMouseExited(row, x, y) {
		row.state = 0;
	}
	onTouchBegan(row) {
		row.state = 2;
	}
	onTouchEnded(row) {
		row.state = 1;
		if (model.devices.length > 1) { 
			let data = {
				button: row,
				items: model.devices.map((device, index) => ({ title:device.title, index })),
			};
			data.items.splice(model.deviceIndex, 1);
			application.add(new ControlsMenu(data));
		}
	}
};

class ControlsMenuBehavior extends Behavior {	
	onClose(layout, index) {
		let data = this.data;
		application.remove(application.last);
		data.button.delegate("onMenuSelected", index);
	}
	onCreate(layout, data) {
		this.data = data;
	}
	onFitVertically(layout, value) {
		let data = this.data;
		let button = data.button;
		let container = layout.first;
		let scroller = container.first;
		let size = scroller.first.measure();
		let y = button.y + button.height + 1
		let height = Math.min(size.height, application.height - y - 20);
		container.coordinates = { left:button.x, width:button.width, top:y, height:height + 10 }
		scroller.coordinates = { left:10, width:button.width - 20, top:0, height:height }
		return value;
	}
	onTouchEnded(layout, id, x, y, ticks) {
		var content = layout.first.first.first;
		if (!content.hit(x, y))
			this.onClose(layout, -1);
	}
};

class ControlsMenuItemBehavior extends ButtonBehavior {
	onTap(item) {
		item.bubble("onClose", this.data.index);
	}
}

class PopupMenuBehavior extends Behavior {	
	onClose(layout, index) {
		let data = this.data;
		application.remove(application.last);
		data.button.delegate("onMenuSelected", index);
	}
	onCreate(layout, data) {
		this.data = data;
	}
	onFitVertically(layout, value) {
		let data = this.data;
		let button = data.button;
		let container = layout.first;
		let scroller = container.first;
		let size = scroller.first.measure();
		let y = Math.max(button.y - ((size.height / data.items.length) * data.selection), 0);
		let height = Math.min(size.height, application.height - y - 20);
		container.coordinates = { left:button.x - 15, width:button.width + 30, top:y, height:height + 10 };
		scroller.coordinates = { left:10, width:button.width + 10, top:0, height:height };
		scroller.first.content(data.selection).first.visible = true;
		return value;
	}
	onTouchEnded(layout, id, x, y, ticks) {
		var content = layout.first.first.first;
		if (!content.hit(x, y))
			this.onClose(layout, -1);
	}
};

class PopupMenuItemBehavior extends ButtonBehavior {
	onTap(item) {
		item.bubble("onClose", item.index);
	}
}

// TEMPLATES

import {
	ScrollerBehavior,
	VerticalScrollbar,
} from "piu/Scrollbars";

import {
	HorizontalSliderBehavior,
	HorizontalSlider,
} from "piu/Sliders";	

import {
	SwitchBehavior,
	Switch,
} from "piu/Switches";	

export var ControlsPane = Container.template($ => ({ 
	left:0, right:0, top:0, bottom:0, 
	contents:[
		Scroller($, { left:0, right:0, top:27, bottom:0, skin:paneBodySkin, active:true, Behavior:ScrollerBehavior, contents: [
			Content($, {}),
			VerticalScrollbar($, {}),
		]}),
		Content($, { left:0, right:0, top:26, height:1, skin:paneBorderSkin, }),
		Row($, {
			left:0, right:0, top:0, height:26, skin:paneHeaderSkin, active:false, Behavior:ControlsHeaderBehavior,
			contents: [
				Content($, { width:30, height:26, skin:controlsMenuGlyphSkin, state:0 }),
				Label($, { left:0, right:0, style:paneHeaderStyle, state:0 }),
			],
		}),
	]
}));

var ControlsMenu = Layout.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true,
	Behavior: ControlsMenuBehavior,
	contents: [
		Container($, { skin:controlsMenuSkin, contents:[
			Scroller($, { clip:true, active:true, contents:[
				Column($, { left:0, right:0, top:0, 
					contents: $.items.map($$ => new ControlsMenuItem($$)),
				}),
			]}),
		]}),
	],
}));

var ControlsMenuItem = Row.template($ => ({
	left:0, right:0, height:30, skin:controlsMenuItemSkin, active:true,
	Behavior:ControlsMenuItemBehavior,
	contents: [
		Label($, {left:20, right:20, height:30, style:controlsMenuItemStyle, string:$.title }),
	]
}));

export var ControlsColumn = Column.template($ => ({ left:0, right:0, top:0 }));

export var Button = Container.template($ => ({
	width:80, height:30, skin:buttonSkin, active:("active" in $) ? $.active : true,
	Behavior: class extends ButtonBehavior {
		onCreate(container, data) {
			super.onCreate(container, data);
			if ("name" in data)
				model.DEVICE.first.behavior[data.name] = container;
		}
		onTouchBegan(container, id, x, y, ticks) {
			super.onTouchBegan(container, id, x, y, ticks);
			let data = this.data;
			if (data.eventDown)
				model.DEVICE.first.delegate(data.eventDown, data);
		}
		onTouchEnded(container, id, x, y, ticks) {
			super.onTouchEnded(container, id, x, y, ticks);
			let data = this.data;
			if (data.eventUp)
				model.DEVICE.first.delegate(data.eventUp, data);
		}
		onTap(container) {
			let data = this.data;
			if (data.event)
				model.DEVICE.first.delegate(data.event, data);
		}
	},
	contents: [
		Label($, { left:0, right:0, height:30, style:buttonStyle, string:$.label }),
]}));

export var ButtonsRow = Row.template(function($) { return {
	left:0, right:0, height:30,
	contents: [
		Label($, { width:120, style:controlNameStyle, string:$.label }),
		$.buttons.map($$ => new Button($$)),
		Content($, { left:0, right:0 }),
	],
}});

var PopupMenu = Layout.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true,
	Behavior: PopupMenuBehavior,
	contents: [
		Container($, { skin:controlsMenuSkin, contents:[
			Scroller($, { clip:true, active:true, contents:[
				Column($, { left:0, right:0, top:0, 
					contents: $.items.map($$ => new PopupMenuItem($$)),
				}),
			]}),
		]}),
	],
}));

var PopupMenuItem = Row.template($ => ({
	left:0, right:0, height:30, skin:controlsMenuItemSkin, active:true,
	Behavior:PopupMenuItemBehavior,
	contents: [
		Content($, { width:20, height:30, skin:dotSkin, visible:false }),
		Label($, { left:0, right:0, height:30, style:controlsMenuItemStyle, string:$.title }),
	]
}));

export var PopupRow = Row.template(function($) { return {
	left:0, right:0, height:30,
	contents: [
		Label($, { width:120, style:controlNameStyle, string:$.label }),
		Container($, { 
			width:160, height:30, active:true, skin:buttonSkin, variant:1,
			Behavior: class extends ButtonBehavior {
				onCreate(container, data) {
					super.onCreate(container, data);
					if ("name" in data)
						model.DEVICE.first.behavior[data.name] = container;
				}
				onDisplaying(container) {
					super.onDisplaying(container);
					let data = this.data;
					this.selection = data.items.findIndex(item => item.value == data.value);
					container.first.string = data.items[this.selection].title;
				}
				onMenuSelected(container, index) {
					if ((index >= 0) && (this.selection != index)) {
						let data = this.data;
						let item = data.items[index];
						this.selection = index;
						data.value = item.value;
						container.first.string = item.title;
						model.DEVICE.first.delegate(data.event, data);
					}
				}
				onTap(container) {
					let data = this.data;
					let it = {
						button: container,
						items: data.items,
						selection: this.selection,
					};
					application.add(new PopupMenu(it));
				}
			},
			contents: [
				Label($, { left:15, right:40, height:30, style:controlsMenuItemStyle, string:$.value }),
			],
		}),
	],
}});

export var SliderRow = Row.template(function($) { return {
	left:0, right:0, height:30,
	contents: [
		Label($, { width:120, style:controlNameStyle, string:$.label }),
		HorizontalSlider($, { 
			width:160, barSkin: sliderBarSkin, buttonSkin: sliderButtonSkin,
			Behavior: class extends HorizontalSliderBehavior {
				onCreate(container, data) {
					super.onCreate(container, data);
					if ("name" in data)
						model.DEVICE.first.behavior[data.name] = container;
				}
				onValueChanged(container) {
					let data = this.data;
					model.DEVICE.first.delegate(data.event, data);
				}
				onValueChanging(container) {
					let data = this.data;
					container.next.string = data.value + data.unit;
				}
			},
		}),
		Label($, { left:0, right:0, style:controlValueStyle, string:$.value + $.unit }),
	],
}});

export var SwitchRow = Row.template(function($) { return {
	left:0, right:0, height:30,
	contents: [
		Label($, { width:120, style:controlNameStyle, string:$.label }),
		Switch($, {
			barSkin: switchBarSkin, buttonSkin: switchButtonSkin,
			Behavior: class extends SwitchBehavior {
				onCreate(container, data) {
					super.onCreate(container, data);
					if ("name" in data)
						model.DEVICE.first.behavior[data.name] = container;
				}
				onValueChanged(container) {
					let data = this.data;
					container.next.string = data.value ? data.on : data.off;
					model.DEVICE.first.delegate(data.event, data);
				}
			},
		}),
		Label($, { left:0, right:0, style:controlValueStyle, string:$.value ? $.on : $.off }),
	],
}});

export var TimerRow = Row.template(function($) { return {
	left:0, right:0, height:30,
	Behavior: class extends Behavior {
		onCreate(row, data) {
			this.data = data;
			if ("interval" in data)
				row.interval = data.interval;
			if ("name" in data)
				model.DEVICE.first.behavior[data.name] = row;
		}
		onDisplaying(row) {
			let container = row.first.next;
			container.next.string = this.secondsToString(0);
		}
		onFinished(row) {
			let data = this.data;
			if ("event" in data)
				model.DEVICE.first.delegate(data.event, data);
		}
		onTimeChanged(row) {
			let data = this.data;
			let container = row.first.next;
			let content = container.first;
			content.width = Math.round(container.width * row.fraction);
			container.next.string = this.secondsToString(Math.round((row.duration - row.time) / 1000));
			if ("tick" in data)
				model.DEVICE.first.delegate(data.tick, data);
		}
		secondsToString(seconds) {
			let string = "";
			let value = Math.floor(seconds / 3600);
			if (value) {
				string = value.toString() + ":";
				seconds %= 3600;
			}
			value = Math.floor(seconds / 60);
			if (value < 10) string += "0";
			string += value.toString() + ":";
			seconds %= 60;
			value = Math.floor(seconds);
			if (value < 10) string += "0";
			string += value.toString();
			return string;
		}
	},
	contents: [
		Label($, { width:120, style:controlNameStyle, string:$.label }),
		Container($, { 
			width:160, height:10, skin:timerSkin,
			contents: [
				Content($, { width:0, right:0, top:0, bottom:0, skin:timerSkin, state:1 }),
			],
		}),
		Label($, { left:0, right:0, style:controlValueStyle }),
	],
}});
