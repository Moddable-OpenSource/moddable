/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

// Handler.Bind("/network/interface/add", class extends Behavior {
// 	onInvoke(handler, message) {
// 		var query = parseQuery(message.query);
// 		model.interfaces[query.name] = query;
// 		shell.distribute("onNetworkInterfacesChanged");
// 	}
// });
// 
// Handler.Bind("/network/interface/remove", class extends Behavior {
// 	onInvoke(handler, message) {
// 		var query = parseQuery(message.query);
// 		delete model.interfaces[query.name];
// 		shell.distribute("onNetworkInterfacesChanged");
// 	}
// });

// ASSETS

import {
	glyphsSkin,
	buttonSkin,
	buttonStyle,
	buttonsSkin,
	
	fieldScrollerSkin,
	
	paneBorderSkin,
	paneHeaderSkin,
	paneHeaderStyle,
	
	preferenceHeaderSkin,
	preferenceRowSkin,
	preferenceCommentStyle,
	preferenceFirstNameStyle,
	preferenceSecondNameStyle,
	preferenceThirdNameStyle,
	preferenceValueStyle,
	
	toggleBarSkin,
	toggleButtonSkin,
	
	noCodeSkin,
} from "assets";	

// BEHAVIORS

import {
	ButtonBehavior,
	ScrollerBehavior,
	HolderColumnBehavior,
	HolderContainerBehavior,
	RowBehavior,
	HeaderBehavior,
	TableBehavior,
} from "behaviors";

class PreferencesColumnBehavior extends Behavior {
	onCreate(column) {
		let preferences = {
			items: [
				{
					Template: PreferencesTable,
					expanded: true,
					name: "BREAK",
					items: [
						{
							Template: ToggleRow,
							comment: "Break when the debuggee starts",
							name: "On Start",
							get value() {
								return model.breakOnStart;
							},
							set value(it) {
								model.toggleBreakOnStart(it);
							},
						},
						{
							Template: ToggleRow,
							comment: "Break when the debuggee throws exceptions",
							name: "On Exceptions",
							get value() {
								return model.breakOnExceptions;
							},
							set value(it) {
								model.toggleBreakOnExceptions(it);
							},
						},
					],
				},
				{
					Template: PreferencesTable,
					expanded: true,
					name: "INSTRUMENTS",
					items: [
						{
							Template: ToggleRow,
							comment: "Show when the debuggee is running, hide when the debuggee is broken",
							name: "Automatically Show & Hide",
							get value() {
								return model.automaticInstruments;
							},
							set value(it) {
								model.automaticInstruments = it;
							},
						},
					],
				},
				{
					Template: PreferencesTable,
					expanded: true,
					name: "NETWORK",
					items: [
						{
							Template: InterfacesRow,
							name: "Interfaces",
						},
						{
							Template: FieldRow,
							name: "Port Number",
							width: 52,
							get value() {
								return model.port;
							},
							set value(it) {
								it = parseInt(it);
								if (isNaN(it))
									return;
								if (it < 1024)
									it = 1024;
								else if (it > 65535)
									it = 65535;
								if (model.port != it) {
									this.FIELD.string = model.port = it	
									application.distribute("onPortChanged");
								}
							},
						},
					],
				},
				{
					Template: PreferencesTable,
					expanded: true,
					name: "LOCATIONS",
					items: model.mappings.map((mapping, index) => ({
						Template: LocationRow,
						index,
						name: mapping.remote + (mapping.alien ? model.alienSeparator : model.separator) + "*",
						value: mapping.locale + model.separator + "*",
					})),
				},
				{
					Template: PreferencesTable,
					expanded: true,
					name: "TEST262",
					items: [
						{
							Template: Test262BaseRow,
							name: "Base",
							get value() {
								return model.test262.base;
							},
							set value(it) {
								model.test262.base = it;
								model.test262.onPreferencesChanged();
							},
						},
						{
							Template: Test262FilterRow,
							name: "Filter",
							get value() {
								return model.test262.filter;
							},
							set value(it) {
								model.test262.filter = it;
								model.test262.onPreferencesChanged();
							},
						},
					],
				},
			]
		}
		preferences.items.forEach(item => column.add(new item.Template(item)));
	}
}

class PreferencesTableBehavior extends TableBehavior {
	addRows(table, expandIt) {
		this.data.items.forEach(item => table.add(new item.Template(item)));
	}
	expand(table, expandIt) {
		var data = this.data;
		var header = table.first;
		data.expanded = expandIt;
		if (expandIt) {
			header.behavior.expand(header, true);
			this.addRows(table);
		}
		else {
			header.behavior.expand(header, false);
			table.empty(1);
		}
	}
	onCreate(table, data) {
		super.onCreate(table, data);
		this.expand(table, data.expanded);
	}
}

class FieldRowBehavior extends RowBehavior {
	onTouchBegan(container, id, x, y, ticks) {
		this.data.FIELD.focus();
		super.onTouchBegan(container, id, x, y, ticks);
	}
	onTap() {
	}
};

class LocationRowBehavior extends RowBehavior {
	changeState(row) {
		super.changeState(row);
		row.last.visible = (this.flags & 1) ? true : false;
	}
	doRemoveMapping(row, index) {
		model.mappings.splice(index, 1);
		row.container.remove(row);
	}
	onTap() {
	}
};

class ToggleRowBehavior extends RowBehavior {
	changeState(row) {
		super.changeState(row);
		row.last.visible = (this.flags & 1) ? true : false;
	}
	onTap() {
	}
}

// TEMPLATES

import {
	HorizontalScrollbar,
	VerticalScrollbar,
} from "piu/Scrollbars";

export var PreferencesView = Container.template($ => ({
	left:0, right:0, top:0, bottom:0,
	contents: [
		Container($, {
			left:0, right:0, top:26, bottom:0, skin:noCodeSkin,
			contents: [
				Content($, { left:0, right:0, top:0, height:1, skin:paneBorderSkin, }),
				Scroller($, {
					left:0, right:0, top:1, bottom:0, clip:true, active:true, 
					Behavior:ScrollerBehavior,
					contents: [
						Column($, {
							left:0, right:0, top:0,
							Behavior:PreferencesColumnBehavior,
						}),
						HorizontalScrollbar($, {}),
						VerticalScrollbar($, {}),
					],
				}),
			],
		}),
		Row($, {
			left:0, right:0, top:0, height:26, skin:paneHeaderSkin, 
			contents: [
				Content($, { width:8 }),
				Label($, { left:0, right:0, style:paneHeaderStyle, string:"PREFERENCES" }),
				Content($, { 
					width:30, height:30, skin:buttonsSkin, variant:6, state:1, active:true, 
					Behavior: class extends ButtonBehavior {
						onTap(button) {
							button.bubble("doCloseFile");
						}
					},
				}),
			],
		}),
	],
}));


var PreferencesTable = Column.template(function($) { return {
	left:0, right:0,
	Behavior: PreferencesTableBehavior,
	contents: [
		Row($, {
			left:0, right:0, height:27, skin:preferenceHeaderSkin, active:true,
			Behavior: HeaderBehavior,
			contents: [
				Content($, { width:0 }),
				Content($, { width:26, top:3, skin:glyphsSkin, state:$.expanded ? 3 : 1, variant:0 }),
				Label($, { left:0, right:0, style:preferenceFirstNameStyle, string:$.name }),
			],
		}),
	],
}});

var FieldRow = Row.template($ => ({
	left:0, right:0, height:26, skin:preferenceRowSkin, active:true,
	Behavior: FieldRowBehavior,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:preferenceSecondNameStyle, string:$.name }),
		Container($, {
			width:$.width, height:26,
			contents: [
				Field($, { 
					anchor:"FIELD", left:0, right:0, top:2, bottom:2, skin:fieldScrollerSkin, style:preferenceValueStyle, string:$.value, 
					Behavior: class extends Behavior {
						onCreate(field, data) {
							this.data = data;
						}
						onEnter(field) {
							this.data.value = field.string;
						}
						onFocused(field) {
							let button = field.container.next;
							button.visible = true;
						}
						onUnfocused(field) {
							let button = field.container.next;
							if (button.state != 2) {
								button.visible = false;
								field.string = this.data.value;
							}
						}
					}
				}),
			],
		}),
		Container($, {
			width:60, skin:buttonSkin, active:true, visible:false,
			Behavior: class extends ButtonBehavior {
				onTap(button) {
					let field = button.previous.first;
					this.data.value = field.string();
					field.focus();
				}
			},
			contents: [
				Label($, { left:0, right:0, style:buttonStyle, string:"Set" }),
			],
		}),
	],
}));

var LocationRow = Row.template($ => ({
	left:0, right:0, height:26, skin:preferenceRowSkin, active:true,
	Behavior: LocationRowBehavior,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:preferenceSecondNameStyle, string:$.name }),
		Label($, { width:180, style:preferenceValueStyle, string:$.value }),
		Content($, { skin:buttonsSkin, variant:5, active:true, visible:false, 
			Behavior: class extends ButtonBehavior {
				onTap(button) {
					button.bubble("doRemoveMapping", this.data.index);
				}
			},
		}),
	],
}));

var InterfacesRow = Row.template($ => ({
	left:0, right:0, height:26, skin:preferenceRowSkin,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:preferenceSecondNameStyle, string:$.name }),
		Label($, { 
			left:0, right:0, style:preferenceValueStyle, 
			Behavior: class extends Behavior {
				onCreate(label) {
					this.onNetworkInterfacesChanged(label);
				}
				onNetworkInterfacesChanged(label) {
					let string = "localhost";
					for (let name in model.interfaces)
						string += ", " + model.interfaces[name].ip;
					label.string = string;	
				}
			},
		}),
	],
}));

var Test262BaseRow = Row.template($ => ({
	left:0, right:0, height:26, skin:preferenceRowSkin, active:true,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:preferenceSecondNameStyle, string:$.name }),
		Label($, { width:180, style:preferenceValueStyle, string:$.value }),
		Container($, {
			width:80, skin:buttonSkin, active:true, name:"onEnter",
			Behavior: class extends ButtonBehavior {
				onEnter(button) {
					var path = this.data.path;
					var dictionary = { message:"Locate test262", prompt:"Open" };
					system.openDirectory(dictionary, path => { 
						if (path) 
							button.previous.string = $.value = path;
					});
				}
			},
			contents: [
				Label($, { left:0, right:0, style:buttonStyle, string:"Locate..." }),
			],
		}),
	],
}));

var Test262FilterRow = Row.template($ => ({
	left:0, right:0, height:26, skin:preferenceRowSkin, active:true,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:preferenceSecondNameStyle, string:$.name }),
		Container($, {
			width:180, height:26,
			contents: [
				Field($, { 
					anchor:"FIELD", left:0, right:0, top:2, bottom:2, skin:fieldScrollerSkin, style:preferenceValueStyle, string:$.value, 
					Behavior: class extends Behavior {
						onCreate(field, data) {
							this.data = data;
						}
						onFocused(field) {
							let button = field.container.next;
							button.visible = true;
						}
						onUnfocused(field) {
							let button = field.container.next;
							if (button.state != 2) {
								button.visible = false;
								field.string = this.data.value;
							}
						}
					}
				}),
			],
		}),
		Container($, {
			width:80, skin:buttonSkin, active:true, visible:false,
			Behavior: class extends ButtonBehavior {
				onTap(button) {
					let field = button.previous.first;
					this.data.value = field.string;
					field.focus();
				}
			},
			contents: [
				Label($, { left:0, right:0, style:buttonStyle, string:"Apply" }),
			],
		}),
	],
}));

var ToggleRow = Row.template(function($) { return {
	left:0, right:0, height:26, skin:preferenceRowSkin, active:true,
	Behavior: ToggleRowBehavior,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:preferenceSecondNameStyle, string:$.name }),
		ToggleButton($, { }),
		Label($, { left:0, right:0, style:preferenceCommentStyle, string:$.comment, visible:false }),
	],
}});

class ToggleButtonBehavior extends Behavior {
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
		let data = this.data;
		this.changeOffset(container, (data.value == 0) ? 0 : (data.value == 1) ? this.size : this.size >> 1);
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
		var value = ((this.offset + delta) == 0) ? 0 : 1;
		if (this.data.value != value) {
			this.data.value = value;
			container.container.bubble("onToggleChanged", this.data);
		}
	}
	onTouchMoved(container, id, x, y, ticks) {
		this.moved = Math.abs(x - this.anchor) >= 8;
		this.changeOffset(container, this.delta + x - this.anchor);
	}
	onValueChanged(container, data) {
		if (this.data == data) {
			let offset = (data.value == 0) ? 0 : (data.value == 1) ? this.size : this.size >> 1;
			if (this.offset != offset) {
				this.anchor = this.offset;
				this.delta = offset - this.offset;
				container.duration = 125 * Math.abs(this.delta) / this.size;
				container.time = 0;
				container.start();
			}
		}
	}
};

var ToggleButton = Container.template($ => ({
	width:50, height:30, active:true,
	Behavior: ToggleButtonBehavior,
	contents: [
		Content($, { left:0, right:0, height:30, skin:toggleBarSkin }),
		Content($, { left:0, width:30, height:30, skin:toggleButtonSkin }),
	],
}));
