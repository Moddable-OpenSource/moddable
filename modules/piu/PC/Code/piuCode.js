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

import { 
	Template,
	Content,
} from "piu/All";

var code = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuCode_create",

	get columnCount() @ "PiuCode_get_columnCount",
	get columnWidth() @ "PiuCode_get_columnWidth",
	get length() @ "PiuCode_get_length",
	get lineCount() @ "PiuCode_get_lineCount",
	get lineHeight() @ "PiuCode_get_lineHeight",
	get resultCount() @ "PiuCode_get_resultCount",
	get selectionBounds() @ "PiuCode_get_selectionBounds",
	get selectionOffset() @ "PiuCode_get_selectionOffset",
	get selectionLength() @ "PiuCode_get_selectionLength",
	get selectionString() @ "PiuCode_get_selectionString",
	get string() @ "PiuCode_get_string",
	get type() @ "PiuCode_get_type",
	
	set string(it) @ "PiuCode_set_string",
	set type(it) @ "PiuCode_set_type",

	colorize(colors) @ "PiuCode_colorize",
	find(pattern, mode) @ "PiuCode_find",
	findAgain(direction) @ "PiuCode_findAgain",
	findBlock(offset) @ "PiuCode_findBlock",
	findLineBreak(offset, forward) @ "PiuCode_findLineBreak",
	findWordBreak(offset, forward) @ "PiuCode_findWordBreak",
	hitOffset(x, y) @ "PiuCode_hitOffset",
	isSpace(offset) @ "PiuCode_isSpace",
	locate(offset) @ "PiuCode_locate",
	select(offset, length) @ "PiuCode_select",
};
export var Code = Template(code);

