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

class CodeViewBehavior extends Behavior {
	canFind(container) {
		return true;
	}
	canFindNext(container) {
		return this.resultCount > 0;
	}
	canFindPrevious(container) {
		return this.resultCount > 0;
	}
	canFindSelection(container) {
		var code = this.data.CODE;
		return code.selectionLength > 0;
	}
	doFind(container) {
		var data = this.data;
		var code = this.data.CODE;
		var findRow = data.FIND;
		code.behavior.find(code, data.findString, data.findMode);
		data.FIND_FOCUS.focus();
		if (!findRow.visible)
			container.run(new FindTransition, container.first, findRow, null, 1);
	}
	doFindNext(container) {
		var code = this.data.CODE;
		code.focus();
		code.findAgain(1);
		code.behavior.onSelected(code);
		code.behavior.onReveal(code);
	}
	doFindPrevious(container) {
		var code = this.data.CODE;
		code.focus();
		code.findAgain(-1);
		code.behavior.onSelected(code);
		code.behavior.onReveal(code);
	}
	doFindSelection(container) {
		var data = this.data;
		var code = data.CODE;
		var findRow = data.FIND;
		data.findSelection = true;
		data.findString = code.selectionString;
		code.behavior.find(code, data.findString, data.findMode);
		var label = data.FIND_FOCUS;
		label.string = data.findString;
		if (!findRow.visible)
			container.run(new FindTransition, container.first, findRow, null, 1);
	}
	onCreate(container, data) {
		this.data = data;
		this.resultCount = 0;
	}
	onCodeSelected(container) {
		var data = this.data;
		var code = this.data.CODE;
		this.resultCount = code.resultCount;
		container.distribute("onFound", this.resultCount);
	}
	onFindDone(container) {
		var data = this.data;
		var code = this.data.CODE;
		var findRow = data.FIND;
		code.behavior.find(code, "", 0);
		if (findRow.visible)
			container.run(new FindTransition, container.first, findRow, null, 0);
	}
	onFindEdited(container) {
		var data = this.data;
		var code = data.CODE;
		code.behavior.find(code, data.findString, data.findMode);
	}
	onKeyDown(container, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		if (c == 9)
			return true;
		if (c == 25)
			return true;
		if (c == 27)
			return true;
		return false;
	}
	onKeyUp(container, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		if (c == 9) {
			var data = this.data;
			if (data.FIND_FOCUS.focused)
				data.CODE.focus();
			return true;
		}
		if (c == 25) {
			var data = this.data;
			if (data.FIND_FOCUS.focused)
				data.CODE.focus();
			return true;
		}
		if (c == 27) {
			this.onFindDone(container);
			return true;
		}
		return false;
	}
	onMouseEntered(container, x, y) {
		this.reveal(container, true);
	}
	onMouseExited(container, x, y) {
		this.reveal(container, false);
	}
	reveal(container, flag) {
		let header = container.last;
		let button = header.last;
		button.visible = flag;
		button = button.previous;
		button.visible = flag;
		button = button.previous;
		button.visible = flag;
	}
};

const CHAR_MODE = 0;
const LINE_MODE = 1;
const WORD_MODE = 2;

class CodeBehavior extends _CodeBehavior {
	canDisableBreakpoint(code, item) {
		let location = code.locate(code.selectionOffset);
		var lines = this.data.LINES;
		var line = lines.content(Math.floor(location.y / code.lineHeight));
		var content = line.first.next;
		if (content.state === 0) {
			return false;
		}
		item.state = (content.variant & 1) ? 0 : 1;
		return true;
	}
	canToggleBreakpoint(code, item) {
		let location = code.locate(code.selectionOffset);
		var lines = this.data.LINES;
		var line = lines.content(Math.floor(location.y / code.lineHeight));
		var content = line.first.next;
		item.state = content.state & 1;
		return true;
	}
	doDisableBreakpoint(code, item) {
		let data = this.data;
		let location = code.locate(code.selectionOffset);
		let at = Math.floor(location.y / code.lineHeight) + 1;
		data.doDisableBreakpoint(data.path, at);
	}
	doToggleBreakpoint(code, item) {
		let data = this.data;
		let location = code.locate(code.selectionOffset);
		let at = Math.floor(location.y / code.lineHeight) + 1;
		data.doToggleBreakpoint(data.path, at);
	}
	find(code, findString, findMode) {
		code.find(findModeToPattern(findMode, findString), findModeToCaseless(findMode));
		this.onSelected(code);
		this.onReveal(code);
	}
	onCreate(code, data, dictionary) {
		this.data = data;
		this.info = null;
		this.notifier = null;

		this.dx = 0;
		this.dy = 0;
		this.acceleration = 0.01;
		this.touchCount = 0;
		this.touchTicks = 0;
		this.touchX = 0;
		this.touchY = 0;
		this.anchor = {
			from:0,
			to:0,
		};
	}
	onDisplaying(code) {
		let lines = this.data.LINES;
		lines.behavior.onLineHeightChanged(lines, code.lineHeight);
		let data = this.data;
		let path = data.path;
		this.onFileChanged(code);
		this.onStateChanged(code, data.state);
		this.info = system.getFileInfo(path);
		this.notifier = new system.DirectoryNotifier(system.getPathDirectory(path), it => this.onDirectoryChanged(code));
		code.focus();
	}
	onDirectoryChanged(code) {
		let data = this.data;
		let path = data.path;
		let info = system.getFileInfo(path);
		if (info) {
			if (this.info.date == info.date)
				return;
			this.info = info;
			this.onStateChanging(code);
			this.onFileChanged(code)
			this.onStateChanged(code, data.state);
		}
		else {
			model.doCloseFile();
		}
	}
	onFileChanged(code) {
		var path = this.data.path;
		code.stop();
		if (path.endsWith(".js") || path.endsWith(".ts") || path.endsWith(".mjs"))
			code.type = "js";
		else if (path.endsWith(".json"))
			code.type = "json";
		else if (path.endsWith(".xml"))
			code.type = "xml";
		else
			code.type = "text";
		let string = system.readFileString(path);
// 		if (string.length > 0x7FFF)
// 			string = string.slice(0, 0x7FFF);
		code.string = string;
		this.insertionOffset = -1;
		this.history = [];
		this.historyIndex = 0;
		var lines = this.data.LINES;
		lines.behavior.onEdited(lines, code);
		code.container.scrollTo(0, 0);
	}
	onReveal(code) {
		code.container.reveal(code.selectionBounds, true);
	}
	onSelected(code) {
		code.bubble("onCodeSelected");
		this.data.at = undefined;
	}
	onStateChanged(code, state) {
		if (state) {
			let line = state.line;
			if (line) {
				let offset = code.hitOffset(0, code.lineHeight * (line - 1));
				let from = code.findLineBreak(offset, false);
				let to = code.findLineBreak(offset, true);
				code.select(from, to - from);
				this.onReveal(code);
			}
			else {
				let selection = state.selection;
				if (selection) {
					code.select(selection.offset, selection.length);
					let scroll = state.scroll;
					if (scroll)
						code.container.scroll = scroll;
					else
						this.onReveal(code);
				}
			}
		}
	}
	onStateChanging(code) {
		this.data.state = {
			scroll: code.container.scroll,
			selection: {
				offset: code.selectionOffset,
				length: code.selectionLength,
			},
		};
	}
	onTimeChanged(code) {
		var scroller = code.container;
		var t = Date.now();
		scroller.scrollBy(this.dx * this.acceleration * (t - this.tx), this.dy * this.acceleration * (t - this.ty));
		this.onTouchSelect(code, this.x, this.y);
	}
	onTouchBegan(code, id, x, y, ticks) {
		code.captureTouch(id, x, y, ticks);
		this.onTouchBeganMode(code, id, x, y, ticks, CHAR_MODE);
	}
	onTouchBeganMode(code, id, x, y, ticks, mode) {
		code.focus();
		if ((ticks - this.touchTicks < 1000) && (this.touchX == x) && (this.touchY == y)) {
			this.touchCount++;
			if (this.touchCount == 4)
				this.touchCount = 1;
		}
		else {
			this.touchCount = 1;
			this.touchX = x;
			this.touchY = y;
		}
		this.touchTicks = ticks;
		if (mode == CHAR_MODE) {
			if (this.touchCount == 2)
				mode = WORD_MODE;
			else if (this.touchCount == 3)
				mode = LINE_MODE;
		}
		this.mode = mode;
		var anchor = this.anchor;
		var bounds = code.bounds;
		var offset = code.hitOffset(x - bounds.x, y - bounds.y);
		var from, to;
		if (this.mode == LINE_MODE) {
			if (shiftKey) {
				if (offset < anchor.to) {
					from = code.findLineBreak(offset, false);
					to = code.selectionOffset + code.selectionLength;
				}
				else {
					from = code.selectionOffset;
					to = code.findLineBreak(offset, true);
				}
			}
			else {
				from = anchor.from = code.findLineBreak(offset, false);
				to = anchor.to = code.findLineBreak(offset, true);
			}
		}
		else if (this.mode == WORD_MODE) {
			if (shiftKey) {
				if (offset < anchor.to) {
					from = code.findWordBreak(offset, false);
					to = code.selectionOffset + code.selectionLength;
				}
				else {
					from = code.selectionOffset;
					to = code.findWordBreak(offset, true);
				}
			}
			else {
				var range = code.findBlock(offset);
				if (range) {
					from = range.from;
					to = range.to;
					anchor.from = anchor.to = offset;
				}
				else {
					from = anchor.from = code.findWordBreak(offset, false);
					to = anchor.to = code.findWordBreak(offset, true);
				}
			}
		}
		else {
			if (shiftKey) {
				if (offset < code.selectionOffset) {
					from = offset;
					to = anchor.from = anchor.to = code.selectionOffset + code.selectionLength;
				}
				else {
					from = anchor.from = anchor.to = code.selectionOffset;
					to = offset;
				}
			}
			else
				from = to = anchor.from = anchor.to = offset;
		}
		code.select(from, to - from);
		this.x = x;
		this.y = y;
	}
	onTouchCancelled(code, id, x, y, ticks) {
	}
	onTouchEnded(code, id, x, y, ticks) {
		code.stop();
		this.onSelected(code);
	}
	onTouchMoved(code, id, x, y, ticks) {
		if ((this.x != x) || (this.y != y))
			this.onTouchSelect(code, x, y);
		this.x = x;
		this.y = y;
		var scroller = code.container;
		var dx = 0, dy = 0;
		if (this.mode != LINE_MODE) {
			if (x < scroller.x) 
				dx = -1;
			else if (scroller.x + scroller.width < x) 
				dx = 1;
		}
		if (y < scroller.y) 
			dy = -1;
		else if (scroller.y + scroller.height < y) 
			dy = 1;
		if (this.dx != dx) {
			this.dx = dx;
			this.tx = Date.now();
		}
		if (this.dy != dy) {
			this.dy = dy;
			this.ty = Date.now();
		}
		if (dx || dy)
			code.start();
		else
			code.stop();
	}
	onTouchSelect(code, x, y) {
		var anchor = this.anchor;
		var bounds = code.bounds;
		var offset = code.hitOffset(x - bounds.x, y - bounds.y);
		if (this.mode == LINE_MODE) {
			if (anchor.from < offset)
				offset = code.findLineBreak(offset, true);
			else
				offset = code.findLineBreak(offset, false);
		}
		else if (this.mode == WORD_MODE) {
			if (anchor.from < offset)
				offset = code.findWordBreak(offset, true);
			else
				offset = code.findWordBreak(offset, false);
		}
		if (anchor.from < offset)
			code.select(anchor.from, offset - anchor.from);
		else
			code.select(offset, anchor.to - offset);
	}
	onUndisplayed(code) {
		this.notifier.close();
		this.notifier = null;
	}
	onUnfocused(code) {
	}
};

class LineNumbersBehavior extends Behavior {
	onBreakpointsChanged(column) {
		let data = this.data;
		let length = column.length;
		let path = data.path;
		let machine = data.currentMachine;
		let container = column.first;
		while (container) {
			let content = container.first;
			while (content) {
				content.state = 0;
				content = content.next;
			}
			container = container.next;
		}
		data.breakpoints.items.forEach(breakpoint => {
			if (breakpoint.path == path) {
				let at = breakpoint.line - 1;
				if ((0 <= at) && (at < length)) {
					let container = column.content(at);
					let content = container.first.next;
					content.state = content.next.state = 1;
					content.variant = breakpoint.enabled ? 0 : 1;
				}
			}
		});
		if (machine && machine.broken) {
			let view = machine.framesView;
			view.lines.forEach(data => {
				if (data.path == path) {
					let at = data.line - 1;
					if ((0 <= at) && (at < length)) {
						let container = column.content(at);
						container.first.state = 1;
					}
				}
			});
		}
	}
	onCreate(column, data) {
		this.data = data;
		this.lineHeight = 0;
	}
	onEdited(column, code) {
		var former = column.length;
		var current = code.lineCount;
		if (former < current) {
			let dictionary = { height: Math.round(code.lineHeight) };
			while (former < current) {
				column.add(LineNumber(former, dictionary));
				former++;
			}
			this.onBreakpointsChanged(column);
		}
		else if (former > current) {
			column.empty(current);
		}
	}
	onLineHeightChanged(column, lineHeight) {
		if (this.lineHeight != lineHeight) {
			this.lineHeight = lineHeight;
			let content = column.first;
			let height = Math.round(lineHeight);
			while (content) {
				content.height = height;
				content = content.next;
			}
		}
	}
	onMachineBroken(column) {
		this.onBreakpointsChanged(column);
	}
	onMachineChanged(column) {
		this.onBreakpointsChanged(column);
	}
	onMachineSelected(column, machine) {
		this.onBreakpointsChanged(column);
	}
	onMachinesChanged(column) {
		this.onBreakpointsChanged(column);
	}
	onTouchBegan(column, id, x, y, ticks) {
		this.data.CODE.focus();
		column.captureTouch(id, x, y, ticks);
		//var code = this.data.CODE;
		//code.behavior.onTouchBeganMode(code, id, x, y, ticks, LINE_MODE);
	}
	onTouchEnded(column, id, x, y, ticks) {
		let at = Math.floor((y - column.y) / this.lineHeight);
		// shiftKey for disabling breakpoint
		model.doToggleBreakpoint(this.data.path, at + 1, shiftKey);
		//code.behavior.onTouchEnded(code, id, x, y, ticks);
	}
	onTouchMoved(column, id, x, y, ticks) {
		//var code = this.data.CODE;
		//code.behavior.onTouchMoved(code, id, x, y, ticks);
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

import {
	FindRow,
	findModeToCaseless,
	findModeToPattern,
} from "FindRow";

export var CodeView = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true,
	Behavior: CodeViewBehavior,
	contents: [
		Container($, {
			left:0, right:0, top:26, bottom:0,
			contents: [
				Content($, { left:0, right:0, top:0, height:1, skin:skins.paneBorder, }),
				Scroller($, {
					left:50, right:0, top:1, bottom:0, skin:skins.background, clip:true, active:true, 
					Behavior: class extends ScrollerBehavior {
						onScrolled(scroller) {
							scroller.next.first.next.scrollTo(0, scroller.scroll.y);
						}
					},
					contents: [
						Code($, { anchor:"CODE", left:0, top:0, skin:skins.code, style:styles.code, active:true, selectable:true, Behavior:CodeBehavior }),
						HorizontalScrollbar($, {}),
						VerticalScrollbar($, {}),
					],
				}),
				Container($, {
					left:0, width:60, top:1, bottom:0, clip:true,
					contents: [
						Content($, { left:0, width:50, top:0, bottom:0, skin:skins.lineNumber, }),
						Scroller($, {
							left:0, width:50, top:0, bottom:0, active:true,
							Behavior: class extends Behavior {
								onMouseScrolled(scroller, dx, dy) {
									scroller.container.previous.scrollBy(-dx, -dy);
								}
							},
							contents: [
								Column($, {
									anchor:"LINES", left:0, right:0, top:0, active:true, 
									Behavior: LineNumbersBehavior,
								}),
							],
						}),
					],
				}),
			],
		}),
		FindRow($, { anchor:"FIND" }),
		Row($, {
			left:0, right:0, top:0, height:26, skin:skins.paneHeader, state:1, active:true, 
			Behavior: class extends Behavior {
				onCreate(row, data) {
					this.data = data;
				}
				onTouchBegan(row) {
					this.data.CODE.focus();
				}
			},
			contents: [
				Content($, { width:8 }),
				PathLayout($, {}),
				IconButton($, {
					variant:10, state:1, active:true, 
					Behavior: class extends ButtonBehavior {
						onTap(button) {
							system.launchPath(this.data.path);
						}
					},
				}),
				IconButton($, { variant:11, state:1, active:true, Behavior:ButtonBehavior, name:"doFind" }),
				IconButton($, { variant:6, state:1, active:true, Behavior:ButtonBehavior, name:"doCloseFile" }),
			],
		}),
	],
}));

var LineNumber = Container.template($ => ({
	left:0, right:0, height:16,
	contents: [
		Content($, { left:0, skin:skins.lineCall, state:0 }),
		Content($, { left:16, skin:skins.lineBreakpoint, state:0 }),
		Label($, { left:0, right:-8, height:16, style:styles.lineNumber, string:++$ }),
	],
}));

var PathLayout = Layout.template($ => ({
	left:0, right:0, clip:true,
	Behavior: class extends Behavior {
		onCreate(layout, data) {
			this.width = styles.pathSpan.measure(data.path).width + 90;
		}
		onAdapt(layout) {
			let text = layout.first;
			let flag = layout.width < this.width;
			text.coordinates = flag ? { width:this.width, right:0 } : { left:0, right:0 };
		}
	},
	contents: [
		Text($, { 
			left:0, right:0, style:styles.pathSpan, active:true,
			Behavior: class extends Behavior {
				onCreate(text, data) {
					let separator = (system.platform == "win") ? "\\" : "/";
					let path = data.path;
					let items = path.split(separator);
					let url = "";
					let name = items.pop();
					items = items.map(string => {
						url += string + "/";
						string += separator;
						let link = new PathLink(url);
						return { link, spans:string };
					});
					items.push({ spans:name, style:styles.pathName });
					text.blocks = [{ spans:items }];
				}
			},
		}),
		IconButton($, { left:0, width:30, height:30, variant:13, visible:false }),
	],
}));

var PathLink = Link.template($ => ({
	Behavior: class extends ButtonBehavior {
		onMouseEntered(content, x, y) {
			content.state = 2;
		}
		onMouseExited(content, x, y) {
			content.state = 1;
		}
		onTap(content) {
			system.launchPath(this.data);
		}
		onTouchBegan(content, id, x, y, ticks) {
			content.state = 3;
			content.captureTouch(id, x, y, ticks);
		}
		onTouchEnded(content, id, x, y, ticks) {
			content.state = 1;
			if (content == content.container.hit(x, y))
				this.onTap(content);
		}
		onTouchMoved(content, id, x, y, ticks) {
			content.state = (content == content.container.hit(x, y)) ? 3 : 2;
		}
	}
}));

import {
	Button,
} from "piu/Buttons";	

export var ErrorView = Container.template($ => ({
	left:0, right:0, top:0, bottom:0,
	contents: [
		Container($, {
			left:0, right:0, top:26, bottom:0,
			contents: [
				Content($, { left:0, right:0, top:0, height:1, skin:skins.paneBorder, }),
				Column($, {
					contents: [
						Label($, { state:1, style:styles.error, string:"File not found!" }),
						Button($, {
							anchor:"BUTTON", height:30, width:80, active:true, string:"Locate...",
							Behavior: class extends ButtonBehavior {
								onTap(button) {
									var path = this.data.path;
									var dictionary = { message:"Locate " + path, prompt:"Open" };
									var extension = system.getPathExtension(path);
									if (extension)
										dictionary.fileType = extension;
									system.openFile(dictionary, path => { if (path) model.mapFile(path); });
								}
							},
						}),
					]
				}),
			],
		}),
		Row($, {
			left:0, right:0, top:0, height:26, skin:skins.paneHeader, state:1, 
			contents: [
				Content($, { width:8 }),
				Label($, { left:0, right:0, style:styles.paneHeader, string:$.path }),
				IconButton($, { variant:6, state:1, active:true, Behavior:ButtonBehavior, name:"doCloseFile" }),
			],
		}),
	],
}));

// TRANSITIONS

class FindTransition extends Transition {
	constructor() {
		super(250);
	}
	onBegin(container, body, findRow, replaceRow, to) {
		let row = findRow.first;
		this.container = container;
		this.body = body;
		this.bodyFrom = body.y;
		this.row = row;
		this.rowFrom = row.y;
		if (findRow.visible) {
			this.bodyTo = findRow.y;
			this.rowTo = findRow.y - row.height;
		}
		else {
			findRow.visible = true;
			this.bodyTo = findRow.y + findRow.height;
			this.rowTo = findRow.y + findRow.height - row.height;
		}
		body.coordinates = { left:0, right:0, top:body.y - container.y, height:container.y + container.height - body.y }
	}
	onEnd(container, body, findRow, replaceRow, to) {
		findRow.visible = to > 0;
		body.coordinates = { left:0, right:0, top:body.y - container.y, bottom:0 };
		if (to == 0)
			this.container.behavior.data.CODE.focus();
		else
			this.container.behavior.data.FIND_FOCUS.focus();
	}
	onStep(fraction) {
		fraction = Math.quadEaseOut(fraction);
		this.row.y = this.rowFrom + ((this.rowTo - this.rowFrom) * fraction);
		var y = this.body.y = this.bodyFrom + ((this.bodyTo - this.bodyFrom) * fraction);
		this.body.height = this.container.y + this.container.height - y;
	}
};


