/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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

import { 
	Template,
	Content,
} from "piu/All";

var code = {
	__proto__: Content.prototype,
	_create($, it) { return native("PiuCode_create").call(this, $, it); },

	get columnCount() { return native("PiuCode_get_columnCount").call(this); },
	get columnWidth() { return native("PiuCode_get_columnWidth").call(this); },
	get length() { return native("PiuCode_get_length").call(this); },
	get lineCount() { return native("PiuCode_get_lineCount").call(this); },
	get lineHeight() { return native("PiuCode_get_lineHeight").call(this); },
	get resultCount() { return native("PiuCode_get_resultCount").call(this); },
	get selectionBounds() { return native("PiuCode_get_selectionBounds").call(this); },
	get selectionOffset() { return native("PiuCode_get_selectionOffset").call(this); },
	get selectionLength() { return native("PiuCode_get_selectionLength").call(this); },
	get selectionString() { return native("PiuCode_get_selectionString").call(this); },
	get string() { return native("PiuCode_get_string").call(this); },
	get type() { return native("PiuCode_get_type").call(this); },
	
	set string(it) { native("PiuCode_set_string").call(this, it); },
	set type(it) { native("PiuCode_set_type").call(this, it); },

	colorize(colors) { return native("PiuCode_colorize").call(this, colors); },
	find(pattern, mode) { return native("PiuCode_find").call(this, pattern, mode); },
	findAgain(direction) { return native("PiuCode_findAgain").call(this, direction); },
	findBlock(offset) { return native("PiuCode_findBlock").call(this, offset); },
	findLineBreak(offset, forward) { return native("PiuCode_findLineBreak").call(this, offset, forward); },
	findWordBreak(offset, forward) { return native("PiuCode_findWordBreak").call(this, offset, forward); },
	hitOffset(x, y) { return native("PiuCode_hitOffset").call(this, x, y); },
	isSpace(offset) { return native("PiuCode_isSpace").call(this, offset); },
	locate(offset) { return native("PiuCode_locate").call(this, offset); },
	select(offset, length) { return native("PiuCode_select").call(this, offset, length); },
};
export var Code = Template(code);

