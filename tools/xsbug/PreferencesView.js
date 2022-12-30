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

import {
	Button,
	PopupButton,
} from "piu/Buttons";	

import {
	Switch,
} from "piu/Switches";	

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
							Template: SwitchRow,
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
							Template: SwitchRow,
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
							Template: SwitchRow,
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
					name: "EXCEPTIONS",
					items: [
						{
							Template: SwitchRow,
							comment: "Show or hide exceptions in console/log pane as they occur",
							name: "Show exceptions",
							get value() {
								return model.showExceptions;
							},
							set value(it) {
								model.showExceptions = it;
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
					name: "NETWORK",
					items: [
						{
							Template: InterfacesRow,
							name: "Interfaces",
						},
						{
							Template: FieldRow,
							name: "Port Number",
							width: 208,
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
					name: "PROFILE",
					items: [
						{
							Template: SwitchRow,
							comment: "Profile when the debuggee starts",
							name: "On Start",
							get value() {
								return model.profileOnStart;
							},
							set value(it) {
								model.toggleProfileOnStart(it);
							},
						},
					],
				},
				{
					Template: PreferencesTable,
					expanded: true,
					name: "SERIAL",
					items: [
						{
							Template: FieldRow,
							name: "Device Path",
							width: 208,
							get value() {
								return model.serialDevicePath;
							},
							set value(it) {
								model.serialDevicePath = it;
							},
						},
						{
							Template: FieldRow,
							name: "Baud Rates",
							width: 208,
							get value() {
								return model.serialBaudRates.join(",");
							},
							set value(it) {
								model.serialBaudRates = it.split(",").map(item => parseInt(item));
							},
						},
					],
				},
				{
					Template: PreferencesTable,
					expanded: true,
					name: "TABS",
					items: [
						{
							Template: SwitchRow,
							comment: "Show Messages tab",
							name: "Messages",
							get value() {
								return model.visibleTabs[1];
							},
							set value(it) {
								model.showTab(1, it);
							},
						},
						{
							Template: SwitchRow,
							comment: "Show Profile tab",
							name: "Profile",
							get value() {
								return model.visibleTabs[2];
							},
							set value(it) {
								model.showTab(2, it);
							},
						},
						{
							Template: SwitchRow,
							comment: "Show Serial tab",
							name: "Serial",
							get value() {
								return model.visibleTabs[3];
							},
							set value(it) {
								model.showTab(3, it);
							},
						},
						{
							Template: SwitchRow,
							comment: "Show Test tab",
							name: "Test",
							get value() {
								return model.visibleTabs[4];
							},
							set value(it) {
								model.showTab(4, it);
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

class PopupRowBehavior extends RowBehavior {
	changeState(row) {
		super.changeState(row);
	}
	onTap() {
	}
}

class SwitchRowBehavior extends RowBehavior {
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
			left:0, right:0, top:26, bottom:0, skin:skins.noCode,
			contents: [
				Content($, { left:0, right:0, top:0, height:1, skin:skins.paneBorder, }),
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
			left:0, right:0, top:0, height:26, skin:skins.paneHeader, state:1, 
			contents: [
				Content($, { width:8 }),
				Label($, { left:0, right:0, style:styles.paneHeader, string:"PREFERENCES" }),
				IconButton($, { 
					variant:6, state:1, active:true, 
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
			left:0, right:0, height:27, skin:skins.preferenceHeader, active:true,
			Behavior: HeaderBehavior,
			contents: [
				Content($, { width:0 }),
				Content($, { width:26, top:5, skin:skins.glyphs, variant:$.expanded ? 3 : 1 }),
				Label($, { left:0, right:0, style:styles.preferenceFirstName, string:$.name }),
			],
		}),
	],
}});

var FieldRow = Row.template($ => ({
	left:0, right:0, height:26, skin:skins.preferenceRow, active:true,
	Behavior: FieldRowBehavior,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:styles.preferenceSecondName, string:$.name }),
		Container($, {
			width:$.width, height:22, skin:skins.fieldScroller,
			contents: [
				Field($, { 
					anchor:"FIELD", left:1, right:1, top:1, bottom:1, skin:skins.field, style:styles.field, string:$.value, 
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
		Button($, {
			height:30, width:60, active:true, visible:false, string:"Set",
			Behavior: class extends ButtonBehavior {
				onTap(button) {
					let field = button.previous.first;
					this.data.value = field.string;
					field.focus();
				}
			}
		}),
	],
}));

var InterfacesRow = Row.template($ => ({
	left:0, right:0, height:26, skin:skins.preferenceRow,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:styles.preferenceSecondName, string:$.name }),
		Label($, { 
			left:0, right:0, style:styles.preferenceValue, 
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

var LocationRow = Row.template($ => ({
	left:0, right:0, height:26, skin:skins.preferenceRow, active:true,
	Behavior: LocationRowBehavior,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:styles.preferenceSecondName, string:$.name }),
		Label($, { width:180, style:styles.preferenceValue, string:$.value }),
		IconButton($, { variant:5, active:true, visible:false, 
			Behavior: class extends ButtonBehavior {
				onTap(button) {
					button.bubble("doRemoveMapping", this.data.index);
				}
			},
		}),
	],
}));

var PopupRow = Row.template(function($) { return {
	left:0, right:0, height:30, skin:skins.preferenceRow, active:true,
	Behavior:PopupRowBehavior,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:styles.preferenceSecondName, string:$.name }),
		PopupButton($, { width:160, height:30, active:true }),
	],
}});

var SwitchRow = Row.template(function($) { return {
	left:0, right:0, height:26, skin:skins.preferenceRow, active:true,
	Behavior: SwitchRowBehavior,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:styles.preferenceSecondName, string:$.name }),
		Switch($, { }),
		Label($, { left:0, right:0, style:styles.preferenceComment, string:$.comment, visible:false }),
	],
}});
