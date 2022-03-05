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

export { ButtonBehavior } from "piu/Buttons";
import { ScrollerBehavior } from "piu/Scrollbars";
export { ScrollerBehavior };

const CHAR_MODE = 0;
const LINE_MODE = 1;
const WORD_MODE = 2;

class CodeState {
	constructor(code) {
		this.string = code.string;
		this.selectionOffset = code.selectionOffset;
		this.selectionLength = code.selectionLength;
	}
};

export class CodeBehavior extends Behavior {
	canClear(code) {
		return code.editable && (code.selectionLength > 0);
	}
	canCopy(code) {
		return code.selectionLength > 0;;
	}
	canCut(code) {
		return code.editable && (code.selectionLength > 0);
	}
	canPaste(code) {
		return code.editable && shell.behavior.hasClipboard();
	}
	canSelectAll(code) {
		return code.length > 0;
	}
	canRedo(code) {
		return this.historyIndex + 1 < this.history.length;
	}
	canUndo(code) {
		return this.historyIndex > 0;
	}
	doClear(code) {
		this.onChanging(code);
		code.insert()
		this.onChanged(code);
		this.onReveal(code);
	}
	doCopy(code) {
		system.setClipboardString(code.selectionString);
	}
	doCut(code) {
		this.doCopy(code);
		this.doClear(code);
	}
	doPaste(code) {
		this.onChanging(code);
		code.insert(shell.behavior.getClipboard())
		this.onChanged(code);
		this.onReveal(code);
	}
	doRedo(code) {
		this.historyIndex++;
		var state = this.history[this.historyIndex];
		code.string = state.string;
		code.select(state.selectionOffset, state.selectionLength);
		this.insertionOffset = -1;
		this.onEdited(code);
		this.onSelected(code);
		this.onReveal(code);
	}
	doSelectAll(code) {
		code.select(0, code.length);
		this.onSelected(code);
	}
	doUndo(code) {
		if (this.historyIndex == this.history.length)
			this.history.push(new CodeState(code));
		this.historyIndex--;
		var state = this.history[this.historyIndex];
		code.string = state.string;
		code.select(state.selectionOffset, state.selectionLength);
		this.insertionOffset = -1;
		this.onEdited(code);
		this.onSelected(code);
		this.onReveal(code);
	}
	filterKey(code, key) {
		if (!key) return null;
		var c = key.charCodeAt(0);
		if (c > 0x000F0000)
			return null;
		if ((1 == c) || (4 == c) || (11 == c) || (12 == c) || (27 == c))
			return null;
		if (this.field && ((c == 3) || (c == 9) || (c == 13) || (c == 25)))
			return null;
		if (code.editable)
			return c;
		if (code.selectable && (((28 <= c) && (c <= 31))))
			return c;
		return null;
	}
	getScroller(code) {
		return code.container;
	}
	onChanging(code) {
		if (this.insertionOffset != code.selectionOffset) {
			this.history.length = this.historyIndex;
			this.history.push(new CodeState(code));
			this.historyIndex++;
		}
	}
	onChanged(code) {
		this.insertionOffset = code.selectionOffset;
		this.onEdited(code);
		this.onSelected(code);
	}
	onCreate(code, data, dictionary) {
		this.data = data;
		this.cursorAnchor = null;
		this.cursorLocation = null;
		this.cursorOffset = null;
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
		this.insertionOffset = -1;
		this.history = [];
		this.historyIndex = 0;
		this.field = ("field" in dictionary) ? dictionary.field: false;
	}
	onCursorAfter(code, offset) {
		var selectionMin, selectionMax, scrollerMin, scrollerMax, dx, dy;
		if (offset < 0)
			offset = 0;
		if (shiftKey) {
			if (offset <= this.cursorAnchor)
				code.select(offset, this.cursorAnchor - offset);
			else
				code.select(this.cursorAnchor, offset - this.cursorAnchor);
			this.cursorOffset = offset;
		}
		else
			code.select(offset, 0)
		this.onSelected(code);
	}
	onCursorBefore(code, sign) {
		if (shiftKey) {
			if (this.cursorOffset == null) 
				this.cursorOffset = (sign < 0) ? code.selectionOffset : code.selectionOffset + code.selectionLength;
			if (this.cursorAnchor == null) 
				this.cursorAnchor = (sign < 0) ? code.selectionOffset + code.selectionLength : code.selectionOffset;
			return this.cursorOffset;
		}
		this.cursorOffset = null;
		this.cursorAnchor = null;
		return (sign < 0) ? code.selectionOffset : code.selectionOffset + code.selectionLength;
	}
	onCursorCancel(code, sign) {
		this.cursorAnchor = null;
		this.cursorLocation = null;
		this.cursorOffset = null;
	}
	onCursorX(code, sign) {
		var offset = this.onCursorBefore(code, sign);
		this.cursorLocation = null;
		if (controlKey) {
			if (sign > 0) {
				offset = code.findLineBreak(offset, sign > 0);
				if (offset < code.length)
					offset--;
			}
			else
				offset = code.findLineBreak(offset, false);
		}
		else if (optionKey)
			offset = code.findWordBreak(offset, sign > 0);
		else if (!code.selectionLength || shiftKey)
			offset += sign;
		this.onCursorAfter(code, offset);
	}
	onCursorY(code, sign) {
		var offset = this.onCursorBefore(code, sign);
		if (controlKey || this.field) {
			offset = (sign > 0) ? code.length : 0;
			this.cursorLocation = null;
		}
		else {
			if (this.cursorLocation == null)
				this.cursorLocation = code.locate(offset);
			this.cursorLocation.y += sign * this.cursorLocation.height;
			if (this.cursorLocation.y < 0)
				this.cursorLocation.y = 0;
			else if (this.cursorLocation.y > code.height)
				this.cursorLocation.y = code.height;
			offset = code.hitOffset(this.cursorLocation.x, this.cursorLocation.y);
		}
		this.onCursorAfter(code, offset);
	}
	onEdited(code) {
	}
	onFocused(code) {
		if (this.field)
			code.select(0, code.length)
	}
	onKeyDown(code, key, repeat, ticks) {
		var charCode = this.filterKey(code, key);
		if (!charCode) return false
		var insertions = null;
		if (charCode == 28) /* left */
			this.onCursorX(code, -1);
		else if (charCode == 29) /* right */
			this.onCursorX(code, 1);
		else if (charCode == 30) /* up */
			this.onCursorY(code, -1);
		else if (charCode == 31) /* down */
			this.onCursorY(code, 1);
		else {
			if (controlKey) return false;
			this.onCursorCancel(code);
			switch (charCode) {
			case 1: /* home */
				code.select(0, 0);
				this.onSelected(code);
				break;
			case 2: /* delete selection */
				insertions = "";
				break;
			case 4: /* end */
				code.select(code.length, 0);
				this.onSelected(code);
				break;
			case 8: /* backspace */
				if (code.selectionLength == 0)
					code.select(code.selectionOffset - 1, 1)
				insertions = "";
				break;
			case 9: /* tab */
				if (code.selectionLength > 0) {
					this.onChanging(code);
					code.tab(true);
					this.onChanged(code);
					this.onReveal(code);
					return true;
				}
				insertions = key;
				break;
			case 25: /* shift tab */
				if (code.selectionLength > 0) {
					this.onChanging(code);
					code.tab(false);
					this.onChanged(code);
					this.onReveal(code);
					return true;
				}
				insertions = "\t";
				break;
			case 3: /* enter */
			case 13: /* return */
				insertions = "\n";
				var from = code.findLineBreak(code.selectionOffset, false);
				if ((from < code.selectionOffset) && code.isSpace(from)) {
					var to = code.findWordBreak(from, true);
					insertions += code.extract(from, to - from);
				}
				break;
			case 127: /* delete */
				if (code.selectionLength == 0)
					code.select(code.selectionOffset, 1)
				insertions = "";
				break;
			default:
				insertions = key;
			}
		}
		if (insertions != null) {
			this.onChanging(code);
			code.insert(insertions);
			this.onChanged(code);
		}
		this.onReveal(code);
		return true;
	}
	onKeyUp(code, key, repeat, ticks) {
		var charCode = this.filterKey(code, key);
		if (!charCode) return false;
		return true;
	}
	onMouseEntered(code, x, y) {
		application.cursor = cursors.iBeam;
	}
	onReveal(code) {
		this.getScroller(code).reveal(code.selectionBounds);
	}
	onSelected(code) {
	}
	onTimeChanged(code) {
		var scroller = this.getScroller(code);
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
		this.onCursorCancel(code);
		this.onSelected(code);
	}
	onTouchMoved(code, id, x, y, ticks) {
		if ((this.x != x) || (this.y != y))
			this.onTouchSelect(code, x, y);
		this.x = x;
		this.y = y;
		var scroller = this.getScroller(code);
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
};

export class CodeScrollerBehavior extends ScrollerBehavior {
	filterKey(scroller, key) {
		if (!key) return null;
		var c = key.charCodeAt(0);
		if ((1 == c) || (4 == c) || (11 == c) || (12 == c))
			return c;
		return null;
	}
	onKeyDown(scroller, key, repeat, ticks) {
		var charCode = this.filterKey(scroller, key);
		if (!charCode) return false;
		var code = scroller.first;
		switch (charCode) {
		case 1:
			scroller.scrollTo(0, 0);
			break;
		case 4:
			scroller.scrollTo(0, 0x7FFFFFFF);
			break;
		case 11:
			scroller.scrollBy(0, code.lineHeight - scroller.height);
			break;
		case 12:
			scroller.scrollBy(0, scroller.height - code.lineHeight);
			break;
		}
		return true;
	}
	onKeyUp(scroller, key, repeat, ticks) {
		var charCode = this.filterKey(scroller, key);
		if (!charCode) return false;
		return true;
	}
	onTouchBegan(scroller, id, x, y, ticks) {
		scroller.captureTouch(id, x, y, ticks);
		var code = scroller.first;
		code.behavior.onTouchBeganMode(code, id, x, y, ticks, CHAR_MODE);
	}
	onTouchEnded(scroller, id, x, y, ticks) {
		var code = scroller.first;
		code.behavior.onTouchEnded(code, id, x, y, ticks);
	}
	onTouchMoved(scroller, id, x, y, ticks) {
		var code = scroller.first;
		code.behavior.onTouchMoved(code, id, x, y, ticks);
	}
};

export class HolderColumnBehavior extends Behavior {
	onCreate(column) {
		this.height = 0;
		this.table = null;
	}
	onScrolled(column) {
		let scroller = column.container;
		let holder = column.next;
		let heldTable = holder.behavior.table;
		let heldHeader = holder.behavior.header;
		
		let min = scroller.y;
		let max = min + scroller.height;
		let top, bottom;
		if (this.height != column.height) {
			if (this.table && (this.table.container == column)) {
				//scroller.scrollTo(0, this.table.y - column.y);
				//scroller.adjust();
			}
			this.height = column.height;
		}
		let table = column.first;
		while (table) {
			top = table.y;
			bottom = top + table.height;
			if ((top < min) && (min < bottom)) {
				break;
			}
			table = table.next;
		}
		if (this.table != table) {
			if (this.table) {
				this.table.behavior.onFlow(this.table, holder);
			}
			this.table = table;
			if (table) {
				table.behavior.onHold(table, holder);
				holder.height = table.first.height + table.last.height;
			}
		}
		if (table)
			holder.first.y = holder.y - Math.max(0, holder.height - bottom + min);
	}
};

export class HolderContainerBehavior extends Behavior {
	onMouseEntered(holder, x, y) {
		let header = holder.first;
		if (header)
			header.behavior.reveal(header, true);
	}
	onMouseExited(holder, x, y) {
		let header = holder.first;
		if (header)
			header.behavior.reveal(header, false);
	}
};

export class RowBehavior extends Behavior {
	changeState(row) {
		switch(this.flags) {
		case 0: row.state = 1; break;
		case 1: row.state = 2; break;
		case 2: case 3: row.state = 3; break;
		default: row.state = 1; break;
		}
	}
	select(row, selectIt) {
		debugger
// 		if (selectIt)
// 			this.flags |= 4;
// 		else
// 			this.flags &= ~4;
// 		this.changeState(row);
	}
	onChanged(row, data) {
		this.data = data;
	}
	onCreate(row, data) {
		this.data = data;
		this.flags = 0;
	}
	onMouseEntered(row, x, y) {
		this.flags |= 1;
		this.changeState(row);
	}
	onMouseExited(row, x, y) {
		this.flags &= ~1;
		this.changeState(row);
	}
	onTap(row) {
		debugger
	}
	onTouchBegan(row) {
		this.flags |= 2;
		this.changeState(row);
	}
	onTouchEnded(row) {
		this.flags &= ~2;
		this.changeState(row);
		this.onTap(row);
	}
};

export class HeaderBehavior extends RowBehavior {
	changeArrowState(row, state) {
		row.first.next.variant = state;
	}
	expand(row, expandIt) {
		this.changeArrowState(row, expandIt ? 3 : 1);
	}
	onCreate(row, data) {
		super.onCreate(row, data);
		this.held = false;
		this.table = null;
	}
	onTap(row) {
		let table = this.held ? this.table : row.container;
		table.behavior.toggle(table);
	}
	onTouchBegan(row) {
		super.onTouchBegan(row);
		this.changeArrowState(row, 2);
	}
	reveal(row, revealIt) {
	}
};

export class TableBehavior extends Behavior {
	expand(table, expandIt) {
		debugger;
	}
	hold(table) {
		debugger;
	}
	onCreate(table, data) {
		this.data = data;
		this.held = false;
		this.header = null;
	}
	onFlow(table, holder) {
		holder.remove(this.header);
		this.header.behavior.table = null;
		this.header.behavior.held = false;
		this.header = null;
		this.held = false;
	}
	onHold(table, holder) {
		this.held = true;
		this.header = this.hold(table);
		this.header.behavior.held = true;
		this.header.behavior.table = table;
		holder.add(this.header);
	}
	onMouseEntered(table, x, y) {
		let header = this.held ? this.header : table.first;
		header.behavior.reveal(header, true);
	}
	onMouseExited(table, x, y) {
		let header = this.held ? this.header : table.first;
		header.behavior.reveal(header, false);
	}
	toggle(table) {
		this.expand(table, !this.data.expanded);
	}
};

export class SpinnerBehavior extends Behavior {
	onCreate(content, data) {
		this.data = data;
		content.duration = 125;
		content.variant = 0;
	}
	onDisplaying(content) {
		content.start();
	}
	onFinished(content) {
		var variant = content.variant + 1;
		if (variant == 24) variant = 0;
		content.variant = variant;
		content.time = 0;
		content.start();
	}
}


