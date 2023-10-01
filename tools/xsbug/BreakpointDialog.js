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

// BEHAVIORS

import { 
	ButtonBehavior, 
	CodeBehavior as _CodeBehavior, 
	ScrollerBehavior,
} from "behaviors";

import {
	Switch,
	SwitchBehavior,
} from "piu/Switches";	

export function EditBreakpoint(breakpoint, x, y) {
	const $ = {
		breakpoint, x, y,
		enabled: {
			get value() {
				return breakpoint.enabled;
			},
			set value(it) {
				if (breakpoint.path)
					model.doEnableDisableBreakpoint(breakpoint.path, breakpoint.line);
				else
					breakpoint.enabled = it;
			}
		},
		name: {
			get value() {
				return breakpoint.path ?? "";
			},
			set value(name) {
				if (breakpoint.path)
					model.doRemoveBreakpoint(breakpoint);
				breakpoint.path = breakpoint.name = name;
				if (breakpoint.path) {
					const former = model.breakpoints.items.find(it => (it.line == 0) && (it.path == name));
					if (former)
						model.doRemoveBreakpoint(former);
					model.doAddBreakpoint(breakpoint);
				}
				$.CLEAR.visible = breakpoint.path ? true : false;
			}
		},
		condition: {
			get value() {
				return breakpoint.condition ?? "";
			},
			set value(it) {
				breakpoint.condition = it;
				if (breakpoint.enabled && breakpoint.path)
					model.doSetBreakpoint(breakpoint);
				application.distribute("onBreakpointsChanged");
			}
		},
		hitCount: {
			get value() {
				return breakpoint.hitCount ?? "";
			},
			set value(it) {
				breakpoint.hitCount = it;
				if (breakpoint.enabled && breakpoint.path)
					model.doSetBreakpoint(breakpoint);
				application.distribute("onBreakpointsChanged");
			}
		},
		trace: {
			get value() {
				return breakpoint.trace ?? "";
			},
			set value(it) {
				breakpoint.trace = it;
				if (breakpoint.enabled && breakpoint.path)
					model.doSetBreakpoint(breakpoint);
				application.distribute("onBreakpointsChanged");
			}
		},
	};
	application.add(new BreakpointDialog($));
}

class BreakpointDialogBehavior extends Behavior {	
	doClearBreakpoint(layout) {
		let data = this.data;
		model.doToggleBreakpoint(data.breakpoint.path, data.breakpoint.line, false);
		application.remove(application.last);
	}
	doCloseDialog(layout) {
		let data = this.data;
		application.remove(application.last);
	}
	onCreate(layout, data) {
		this.data = data;
		this.once = true;
	}
	onDisplaying(layout) {
		let field;
		if (this.data.breakpoint.id & 2)
			field = this.data.HEADER.next.first.next.next.first;
		else
			field = this.data.HEADER.next.next.first.next.next.first;
		field.focus();
	}
	onFitVertically(layout, value) {
		if (this.once) {
			this.once = false;
			let data = this.data;
			let button = data.button;
			let container = layout.first;
			let scroller = container.first;
			let size = scroller.first.measure();
			let x, y, width, height;
			if ((data.x !== undefined) && (data.y !== undefined)) {
				x = Math.max(data.x, 0);
				width = Math.min(560, application.width - x - 30);
				y = Math.max(data.y, 0);
				height = Math.min(size.height, application.height - y - 20);
			}
			else {
				width = Math.min(560, application.width - 30);
				height = Math.min(size.height, application.height - 30);
				x = (application.width - width) >> 1;
				y = (application.height - height) >> 1;
			}
			container.coordinates = { left:x - 8, width:width + 30, top:y - 8, height:height + 10 };
			scroller.coordinates = { left:10, width:width + 10, top:0, height:height };
			return value;
		}
		this.doCloseDialog(layout);
	}
	onTouchEnded(layout, id, x, y, ticks) {
		var content = layout.first;
		if (!content.hit(x, y))
			this.doCloseDialog(layout);
	}
};

var BreakpointDialog = Layout.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true,
	Behavior: BreakpointDialogBehavior,
	contents: [
		Container($, { skin:skins.popupMenuShadow, contents:[
			Scroller($, { clip:true, active:true, skin:skins.lineNumber, contents:[
				BreakpointColumn($, { }),
			]}),
		]}),
	],
}));

var BreakpointColumn = Column.template($ => ({
	left:0, right:0, top:0, 
	contents: [
		Row($, {
			anchor:"HEADER", left:0, right:0, height:30, skin:skins.tableHeader,
			contents: [
				Content($, { width: 2 }),
				IconButton($, { variant:6, name:"doCloseDialog" }),
				Content($, { width: 2 }),
				Label($, { width:80, style:styles.tableHeader, string:($.breakpoint.id & 2) ? "FUNCTION BREAKPOINT" : "BREAKPOINT" }),
				Content($, { left:0, right:0 }),
				Switch($.enabled, { 
					Behavior: class extends SwitchBehavior {
						onValueChanged(control) {
							control.next.string = this.data.value ? "Enabled" : "Disabled"
						}
					},
				}),
				Label($, { width:60, style:styles.tableRow, string:$.enabled.value ? "Enabled" : "Disabled" }),
				IconButton($, { anchor:"CLEAR", variant:5, name:"doClearBreakpoint", visible:$.breakpoint.path ? true: false }), 
				Content($, { width: 2 }),
			],
		}),
		($.breakpoint.id & 2) ? BreakpointRow($.name, { placeholder:"FUNCTION NAME", string:"Function", variant:0 }) : BreakpointPathLineRow($),
		BreakpointRow($.condition, { placeholder:"CONDITION EXPRESSION e.g. x == 0", string:"Condition", variant:1 }),
		BreakpointRow($.hitCount, { placeholder:"HIT COUNT e.g. <1 or <=2 or =3 or >4 or >=5 or %6", string:"Hit Count", variant:2 }),
		BreakpointRow($.trace, { placeholder:"TRACE EXPRESSION e.g. `x = ${x}`", string:"Trace", variant:3 }),
		Content($, { height:4 }),
	],
}));

var BreakpointPathLineRow = Row.template(($, _) => ({
	left:0, right:0, height:30,
	contents: [
		Content($, { width: 32 }),
		Label($, { style:styles.breakpointRowName, string:$.breakpoint.path }),
		Label($, { style:styles.breakpointRowLine, string:" (" + $.breakpoint.line + ")" }),
	],
}));

var BreakpointRow = Row.template(($, _) => ({
	left:0, right:0, height:30,
	contents: [
		Content($, { width: 6 }),
// 		Label($, { left:0, width:80, style:styles.tableRow, string:_.string }),
		Content($, { width: 20, skin:skins.breakpointGlyphs, variant:_.variant }),
		Container($, {
			left:4, right:4, top:4, bottom:4, skin:skins.fieldScroller,
			contents: [
				Field($, { left:1, right:42, top:1, bottom:1, clip:true, active:true, skin:skins.field, style:styles.field, placeholder:_.placeholder,
					Behavior: class extends Behavior {
						onCreate(field, data) {
							this.data = data;
							field.string = data.value;
						}
						onEnter(field) {
							let button = field.next;
							if (button.visible)
								button.defer("onTap");
						}
// 						onFocused(field) {
// 							let button = field.next;
// 							button.visible = true;
// 						}
						onStringChanged(field) {
					// 		var data = this.data;
					// 		data.value = field.string;
							let button = field.next;
							button.visible = (this.data.value != field.string)
						}
// 						onUnfocused(field) {
// 							let button = field.next;
// 							if (button.state != 2) {
// 								button.visible = false;
// 								field.string = this.data.value;
// 							}
// 						}
					},
				}),
				Container($, {
					width:40, right:1, top:1, bottom:1, active:true, visible:false,
					Behavior: class extends ButtonBehavior {
						onTap(button) {
							let field = button.previous;
							this.data.value = field.string;
							field.focus();
							button.visible = false;
						}
					},
					contents: [
						RoundContent($, { left:1, right:1, top:1, bottom:1, radius:3, skin:skins.breakpointButton }),
						Label($, { left:1, right:1, top:1, bottom:1, style:styles.breakpointButton, string:"Set" }),
					],
				}),
 			],
		}),
		Content($, { width: 2 }),
// 		Button($, {
// 			height:30, width:52, active:true, visible:false, string:"Set",
// 			Behavior: class extends ButtonBehavior {
// 				onTap(button) {
// 					let field = button.previous.first;
// 					this.data.value = field.string;
// 					field.focus();
// 				}
// 			}
// 		}),
	],
}));
