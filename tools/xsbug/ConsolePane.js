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
	ScrollerBehavior,
	CodeBehavior, 
	RowBehavior,
} from "behaviors";

class ConsolePaneBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onMachineDeselected(container, machine) {
		let scroller = container.first.first;
		if (!machine) machine = this.data; 
		machine.consoleScroll = scroller.scroll;
	}
	onMachineLogged(container, string, colors) {
		let scroller = container.first.first;
		let code = scroller.first;
		code.behavior.doLog(code, string, colors);
		scroller.reveal(code.selectionBounds);
	}
	onMachineSelected(container, machine) {
		let scroller = container.first.first;
		let code = scroller.first;
		if (!machine) machine = this.data; 
		code.behavior.doLog(code, machine.consoleText, machine.consoleLines);
		scroller.scroll = machine.consoleScroll;
	}
	onMouseEntered(container, x, y) {
		this.reveal(container, true);
		return true;
	}
	onMouseExited(container, x, y) {
		this.reveal(container, false);
		return true;
	}
	reveal(container, flag) {
		let header = container.last;
		let button = header.last;
		button.visible = flag;
	}
};

class ConsoleCodeBehavior extends CodeBehavior {
	doLog(code, string, colors) {
		this.colors = colors;
		code.string = string;
		code.colorize(colors);
		code.select(code.length, 0);
	}
	onCreate(code, data, dictionary) {
		super.onCreate(code, data, dictionary);
		this.colors = [];
	}
	onMouseMoved(code, x, y) {
		let bounds = code.bounds;
		let offset = code.findLineBreak(code.hitOffset(x - bounds.x, y - bounds.y), false);
		let color = this.colors.find(color => color.offset == offset && color.path !== undefined);
		application.cursor = color ? cursors.link :  cursors.iBeam;
	}
	onTouchEnded(code, id, x, y, ticks) {
		super.onTouchEnded(code, id, x, y, ticks);
		if ((this.mode == 0) && (code.selectionLength == 0)) {
			let offset = code.findLineBreak(code.selectionOffset, false);
			let color = this.colors.find(color => color.offset == offset && color.path !== undefined);
			if (color) {
				model.selectFile(color.path, { line:color.line });
			}
		}
	}
	onSelected(code) {
		code.bubble("onCodeSelected");
	}
};

class ConsoleHeaderBehavior extends RowBehavior {
	changeArrowState(row, state) {
		row.first.variant = state;
	}
	onDisplaying(row) {
		let divider = row.container.container.next;
		this.changeArrowState(row, (divider.behavior.status) ? 3 : 1);
	}
	onDividerChanged(row, divider) {
		if (divider == row.container.container.next)
			this.changeArrowState(row, (divider.behavior.status) ? 3 : 1);
	}
	onTouchBegan(row) {
		super.onTouchBegan(row);
		this.changeArrowState(row, 2);
	}
	onTap(row) {
		application.delegate("doToggleConsole");
	}
};

// TEMPLATES

import {
	Code
} from "piu/Code";

import {
	HorizontalScrollbar,
	VerticalScrollbar,
} from "piu/Scrollbars";

export var ConsolePane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true,
	Behavior: ConsolePaneBehavior,
	contents: [
		Container($, {
			left:0, right:0, top:0, bottom:0,
			contents: [
				Scroller($, {
					left:0, right:0, top:27, bottom:0, skin:skins.background, clip:true, active:true, Behavior:ScrollerBehavior,
					contents: [
						Code($, { left:0, top:0, skin:skins.code, style:styles.code, active:true, selectable:true, Behavior:ConsoleCodeBehavior }),
						HorizontalScrollbar($, {}),
						VerticalScrollbar($, {}),
					],
				}),
				Content($, { left:0, right:0, top:26, height:1, skin:skins.paneBorder, }),
			],
		}),
		Row($, {
			left:0, right:0, top:0, height:26, skin:skins.paneHeader, state:1, active:true, Behavior:ConsoleHeaderBehavior,
			contents: [
				Content($, { width:30, height:26, skin:skins.glyphs }),
				Label($, { 
					left:0, right:0, style:styles.paneHeader, string:"LOG",
					Behavior: class extends ButtonBehavior {
						onMachineSelected(label, machine) {
							if (machine)
								label.string = "CONSOLE";
							else
								label.string = "LOG";
						}
					},
				}),
				IconButton($, {
					variant:5, active:true, visible:false, 
					Behavior: class extends ButtonBehavior {
						onTap(button) {
							button.bubble("doClearConsole");
						}
					},
				}),
			],
		}),
	],
}));
