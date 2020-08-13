/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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

import {} from "piu/MC";
import Timeline from "piu/Timeline";
import {BottomKeyRowBehavior, ExpandingKeyRowBehavior, ToggleModes, Keys, Colors, KeyRowBehavior, KeyboardBehavior} from "common/keyboard";

const TOPMARGIN = 4;
const KEYHEIGHT = 36;
const KEYWIDTHS = Object.freeze([28, 30, 32, 34, 36, 38, 40]);
const KEYOFFSETS = Object.freeze([0, 31, 64, 99, 136, 175, 216]);

const RowTexture0 = Texture.template(Object.freeze({ path:"horz-7pt-28w.png" }));
const RowTexture1 = Texture.template(Object.freeze({ path:"horz-7pt-30w.png" }));
const RowTexture2 = Texture.template(Object.freeze({ path:"horz-7pt-32w.png" }));
const RowTexture3 = Texture.template(Object.freeze({ path:"horz-7pt-34w.png" }));
const RowTexture4 = Texture.template(Object.freeze({ path:"horz-7pt-36w.png" }));
const RowTexture5 = Texture.template(Object.freeze({ path:"horz-7pt-38w.png" }));
const RowTexture6 = Texture.template(Object.freeze({ path:"horz-7pt-40w.png" }));

const BottomRowTexture = Texture.template(Object.freeze({ path:"horz-bottom-row.png" }));
const GlyphsTexture = Texture.template(Object.freeze({ path:"horz-glyph-strip.png" }));

class HorizontalBottomKeyRowBehavior extends BottomKeyRowBehavior {
	onDraw(port) {
		let keyboard = this.keyboard;
		let keyWidth = keyboard.keyWidth;
		let keyHeight = keyboard.keyHeight;
		let toggleMode = keyboard.toggleMode;
		let expanded = keyboard.expanded;
		let texture = keyboard.keyBottomRowTexture;
		let offsets = this.data.x;
		let widths = this.data.widths;
		let tracker = this.tracker;
		let x, color;
		
		let glyphTexture = new GlyphsTexture;
		
		x = offsets[0];
		color = (toggleMode == ToggleModes.SHIFT ? Colors.KEYTOGGLED : Colors.SPECIALKEY);
		port.drawTexture(texture, color, x, 0, x, 0, widths[0], keyHeight);
		port.drawTexture(glyphTexture, Colors.TEXTDOWN, x + 14, 8, 0, 0, 14, 20);

		x = offsets[1];
		color = (toggleMode == ToggleModes.ALT ? Colors.KEYTOGGLED : Colors.SPECIALKEY);
		port.drawTexture(texture, color, x, 0, x, 0, widths[1], keyHeight);
		port.drawTexture(glyphTexture, Colors.TEXTDOWN, x + (widths[1] >> 1) - 16, 8, 26, 0, 32, 20);

		x = offsets[2];
		color = (tracker && tracker.down ? Colors.KEYDOWN : Colors.KEY);
		port.drawTexture(texture, color, x, 0, x, 0, widths[2], keyHeight);

		x = offsets[3];
		color = (tracker && tracker.down ? Colors.KEYDOWN : Colors.SPECIALKEY);
		port.drawTexture(texture, color, x, 0, 0, 0, widths[0], keyHeight);
		port.drawTexture(glyphTexture, Colors.TEXTDOWN, x + (widths[3] >> 1) - 7, 8, 14, 0, 13, 20);

		x = offsets[4];
		port.drawTexture(texture, color, x, 0, 0, 0, widths[0], keyHeight);
		if (!expanded)
			port.drawString(keyboard.ok, port.container.style, Colors.TEXTDOWN, x + ((widths[4] - keyboard.okMeasure.width - 4) >> 1), ((keyHeight - keyboard.okMeasure.height + 2) >> 1), widths[4], keyboard.okMeasure.height);
		else
			port.drawTexture(glyphTexture, Colors.TEXTDOWN, x + (widths[4] >> 1) - 17, 10, 58, 0, 32, 20);

		if (tracker && !tracker.down) {
			delete this.tracker;
		}
	}
}
Object.freeze(HorizontalBottomKeyRowBehavior.prototype);

class HorizontalKeyboardBehavior extends KeyboardBehavior {
	onCreate(container, $, data) {
		super.onCreate(container, $, data);
		this.keyRowTexture = [new RowTexture0, new RowTexture1, new RowTexture2, new RowTexture3, new RowTexture4, new RowTexture5, new RowTexture6];
		this.keyBottomRowTexture = new BottomRowTexture;
		this.keyHeight = KEYHEIGHT;
		this.keyWidths = KEYWIDTHS;
		this.keyOffsets = KEYOFFSETS;
		this.innerKeyGap = 3;
	}
}
Object.freeze(HorizontalKeyboardBehavior.prototype);

const KeyRow = Port.template($ => ({
	left:0, right:0, top:TOPMARGIN, height:KEYHEIGHT, active:true
}));

const ExpandingKeyRow = KeyRow.template($ => ({
	Behavior:ExpandingKeyRowBehavior,
}));

const BottomKeyRow = KeyRow.template($ => ({
	left:6, right:7, 
	Behavior:HorizontalBottomKeyRowBehavior,
}));

export const HorizontalExpandingKeyboard = Column.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true,
	Behavior:HorizontalKeyboardBehavior,
	contents: [
		ExpandingKeyRow(Keys[ToggleModes.NONE][0], { index:0, x:6 }),
		ExpandingKeyRow(Keys[ToggleModes.NONE][1], { index:1, x:22 }),
		ExpandingKeyRow(Keys[ToggleModes.NONE][2], { index:2, x:6 }),
		BottomKeyRow(Keys[3][0], { index:3, x:Uint16Array.of(0, 43, 96, 224, 267), widths:Uint8Array.of(40, 50, 125, 40, 40) }),
	]
}));
