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
import Timer from "timer";

const SUBMITDEFAULT = "OK";

const TIMELINE_DURATION = 220;
const TIMELINE_DELAY = 30;
const EXPANSION_INTERVAL = 33;
const KEY_EDGE = 2;

const ToggleModes = {
	NONE: 0,
	SHIFT: 1,
	ALT: 2,
};
Object.freeze(ToggleModes);

const Keys = [
	["qwertyuiop", "asdfghjkl", "zxcvbnm',."],
	["QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM\"<>"],
	["1234567890", "#$%&*()_@", "!?/\\;:=+-\""],
	["** \b\r"]
];
Object.freeze(Keys, 1);

const Colors = {
	BACKGROUND: "#5b5b5b",
	KEY: "#d8d8d8",
	SPECIALKEY: "#999999",
	TEXT: "#000000",
	TEXTDOWN: "#ffffff",
	KEYDOWN: "#97a2b5",
	KEYTOGGLED: "#77be29",
};
Object.freeze(Colors);

const KeyPressKeyColors = ["#d8d8d8","#cdcfd2","#c2c6cc","#b8bdc7","#adb4c1","#a2abbb","#97a2b5"];
Object.freeze(KeyPressKeyColors, 1);

const KeyPressTextColors = ["#000000","#2b2b2b","#555555","#808080","#aaaaaa","#d5d5d5","#ffffff"];
Object.freeze(KeyPressTextColors, 1);

class KeyRowBehavior extends Behavior {
	onCreate(port, keys, data) {
		this.keys = keys;
		this.data = data;
		this.length = this.keys.length;
		this.widths = new Uint8Array(this.length);
		this.skipClip = false;
	}
	onDisplaying(port) {
		let container = port.container;
		this.keyboard = container.behavior;
		this.style = container.style;
		this.onKeysChanged(port, Keys[this.keyboard.toggleMode]);
	}
	onDraw(port) {
		debugger;
	}
	onKeysChanged(port, keys) {
		this.keys = keys[this.data.index];
		let keyboard = this.keyboard;
		let metrics = keyboard.metrics;
		let length = this.length;
		let chars = this.chars = this.keys.split("");
		let widths = this.widths;
		for (let i = 0; i < length; ++i) {
			let key = chars[i];
			chars[i] = key;
			widths[i] = metrics[key.charCodeAt()];
		}
		port.invalidate();
	}
	onTouchBegan(port, id, x, y) {
		if (port.container.running) return;
		let index = this.data.index;
		let keyboard = this.keyboard;
		let expanded = keyboard.expanded;
		if ((index < 3) && !expanded) {
			let left = this.data.x;
			let keyWidths = keyboard.keyWidths;
			let unzoomedKeyWidth = keyWidths[0];
			let length = this.length;
			let innerKeyGap = keyboard.innerKeyGap;
			let expansionStates = keyboard.expansionStates;
			let unzoomedRowWidth = (unzoomedKeyWidth * length) + ((length - 1) * innerKeyGap);
			let keyPlusGapWidth = unzoomedKeyWidth + innerKeyGap;
			let edgeWidth = unzoomedKeyWidth + innerKeyGap + (unzoomedKeyWidth >> 1);
			keyboard.flag = 0;
			for (let i = 1; i < expansionStates; ++i) {
				let xx = x;
				let x2 = 0;
				let zoomedKeyWidth = keyWidths[i];
				let expandedKeyWidthDelta = zoomedKeyWidth - unzoomedKeyWidth;
				let distance;
				
				// first or third row
				if (0 == index || 2 == index) {
					// left of right edge of 'w' or 'x' key
					if ((x - left) < edgeWidth + (unzoomedKeyWidth >> 1)) {
						// left of center point of 'w' or 'x' key
						if ((x - left) < edgeWidth) {
							xx = 0;
						}
						// between center point and right edge of 'w' or 'x' key
						else {
							distance = (x - left) - edgeWidth - (unzoomedKeyWidth >> 1);
							xx = Math.quadEaseOut(distance/(unzoomedKeyWidth>>1)) + unzoomedKeyWidth + 1;
						}
					}
					// right of center point of 'o' or ',' key
					else if ((x - left) > unzoomedRowWidth - edgeWidth) {
						keyboard.flag = 1;
						xx = left + unzoomedRowWidth;
						x2 = left + unzoomedRowWidth - unzoomedKeyWidth;
					}
					else {
						distance = unzoomedRowWidth - edgeWidth - (x - left);
						// between center point and left edge of 'o' or ',' key
						if (distance < (unzoomedKeyWidth >> 1)) {
							xx = (unzoomedRowWidth * Math.quadEaseOut(((unzoomedKeyWidth - distance)/unzoomedKeyWidth))) + 1;
						}
					}
				}

				// second row
				else {
					// left of center point of 'a' key
					if (x - left - 1 < (unzoomedKeyWidth >> 1)) {
						xx = left;
					}
					// right of right edge of 'l' key
					else if ((x - left) > unzoomedRowWidth) {
						let firstRow = port.container.content(0);
						let firstRowBehavior = firstRow.behavior;
						let firstRowData = firstRowBehavior.data;
						let firstRowLength = firstRowBehavior.length;
						keyboard.flag = 1;
						xx = firstRowData.x + left + (unzoomedKeyWidth * firstRowLength) + ((firstRowLength - 1) * innerKeyGap);
						x2 = left + unzoomedRowWidth;
					}
					// right of center point of 'l' key
					else if (unzoomedRowWidth - (x - left - 1) < (unzoomedKeyWidth >> 1)) {
						keyboard.flag = 1;
						xx = port.width - 1 + left;
						x2 = left + unzoomedRowWidth;
					}
				}
				let rowX = -Math.floor((expandedKeyWidthDelta * (xx - left) / keyPlusGapWidth));
				keyboard.rowX[i] = rowX;
				if (0 != x2)
					keyboard.rowX2[i] = -Math.floor((expandedKeyWidthDelta * (x2 - left) / keyPlusGapWidth));
			}
			keyboard.startRowExpansion(port.container);
		}
		else
			this.keyHit(port, x, y, true);
	}
	onTouchEnded(port, id, x, y) {
		if (!port.container.running)
			this.keyHit(port, x, y, false);
	}
	keyHit(port, x, y, down) {
		debugger;
	}
}
Object.freeze(KeyRowBehavior.prototype);

class ExpandingKeyRowBehavior extends KeyRowBehavior {
	onDraw(port) {
		let tracker = this.tracker;
		let length = this.length;
		let keyboard = this.keyboard;
		let metrics = keyboard.metrics;
		let keyHeight = keyboard.keyHeight;
		let keyWidths = keyboard.keyWidths;
		let expanded = keyboard.expanded;
		let innerKeyGap = keyboard.innerKeyGap;
		let expandState = keyboard.expandState;
		let texture = keyboard.keyRowTexture[expandState];
		let rowX = keyboard.rowX;
		let rowX2 = keyboard.rowX2;
		let keyWidth = keyWidths[expandState];
		let keyPlusGapWidth = keyWidth + innerKeyGap;
		let width = length * keyPlusGapWidth;
		let index = this.data.index;
		let style = this.style;
		let chars = this.chars;
		let widths = this.widths;
		let charHeight = metrics.height;
		let fading;
		let fadeKey = keyboard.fadeKey;
		let x, color, fadeTextColor;
		//++keyboard.frames;

		x = this.data.x + ((keyboard.flag && (1 == index)) ? rowX2[expandState] : rowX[expandState]);
			
		if (tracker) {
			x += (tracker.index * keyPlusGapWidth);
			color = tracker.down ? Colors.KEYDOWN : Colors.KEY;
			length = tracker.index + 1;
		}
		else {
			color = Colors.KEY;
		}

		port.fillTexture(texture, color, x, 0, width, keyHeight, 0, 0, keyPlusGapWidth, keyHeight);
		
		if (fadeKey[0] == index) {
			fading = fadeKey[1];
			if (!this.skipClip)
				port.pushClip(fadeKey[1] * keyPlusGapWidth + x, 0, keyWidth + innerKeyGap, keyHeight);
			port.fillTexture(texture, KeyPressKeyColors[expandState], x, 0, width, keyHeight, 0, 0, keyPlusGapWidth, keyHeight);
			if (!this.skipClip)
				port.popClip();
		}

		color = (tracker && tracker.down) ? Colors.TEXTDOWN : Colors.TEXT;
		fadeTextColor = KeyPressTextColors[expandState];
		
		for (let i = tracker ? tracker.index : 0; i < length; ++i, x += keyWidth + innerKeyGap) {
			const charWidth = widths[i];
			port.drawString(chars[i], style, (i === fading) ? fadeTextColor : color, x + ((keyWidth - charWidth) >> 1), 3, charWidth, charHeight);
		}
		if (tracker && !tracker.down) {
			this.tracker = null;
		}
		this.skipClip = false;
	}
	keyHit(port, x, y, down) {
		let length = this.length;
		let keyboard = this.keyboard;
		let keyHeight = keyboard.keyHeight;
		let expanded = keyboard.expanded;
		let expandState = keyboard.expandState;
		let keyWidths = keyboard.keyWidths;
		let keyWidth = keyWidths[expandState];
		let innerKeyGap = keyboard.innerKeyGap;
		let left = this.data.x + ((keyboard.flag && (1 == this.data.index)) ? keyboard.rowX2[expandState] : keyboard.rowX[expandState]);
		if (down) {
			if (x > left) {
				let right = left + (keyWidth * length) + ((length - 1) * innerKeyGap);
				if (x < right) {
					let keyPlusGapWidth = keyWidth + innerKeyGap;
					let xx = (x - left) % keyPlusGapWidth;
					if ((xx >= KEY_EDGE) && (xx <= keyWidth - KEY_EDGE)) {
						let index = Math.floor((x - left) / keyPlusGapWidth);
						this.tracker = { index, down };
						keyboard.fadeKey[0] = this.data.index;
						keyboard.fadeKey[1] = index;
                    	port.invalidate(left + (index * (keyWidth + innerKeyGap)), 0, keyWidth, keyHeight);
    				}
				}
			}
		}
		else {
			if (this.tracker) {
				port.bubble("onKeyUp", this.chars[this.tracker.index]);
				this.tracker.down = false;
				this.skipClip = true;
				port.invalidate(left + (this.tracker.index * (keyWidth + innerKeyGap)), 0, keyWidth, keyHeight);
			}
		}
	}
}
Object.freeze(ExpandingKeyRowBehavior.prototype);

class BottomKeyRowBehavior extends KeyRowBehavior {
	onKeysChanged(port, keys) {
	}
	keyHit(port, x, y, down) {
		let keyboard = this.keyboard;
		let keyWidth = keyboard.keyWidth;
		let keyHeight = keyboard.keyHeight;
		let offsets = this.data.x;
		let widths = this.data.widths;
		if (down) {
			let length = offsets.length;
			for (let index = length - 1; index >= 0; --index) {
				if (x > offsets[index] && x < offsets[index] + widths[index]) {
					if (0 == index) {
						keyboard.toggleMode = (ToggleModes.SHIFT == keyboard.toggleMode ? ToggleModes.NONE : ToggleModes.SHIFT);
					}
					else if (1 == index) {
						keyboard.toggleMode = (ToggleModes.ALT == keyboard.toggleMode ? ToggleModes.NONE : ToggleModes.ALT);
					}
					if (index <= 1) {
						port.container.distribute("onKeysChanged", Keys[keyboard.toggleMode]);
						port.invalidate(0, 0, offsets[1] + widths[1], keyHeight);
					}
					else {
						this.tracker = { index, down };
						port.invalidate(offsets[index], 0, widths[index], keyHeight);
					}
					break;
				}
			}
		}
		else {
			if (this.tracker) {
				let index = this.tracker.index;
                let key = this.keys.charAt(index);
				if ('*' != key)
					port.bubble("onKeyUp", key);
				this.tracker.down = false;
				port.invalidate(offsets[index], 0, widths[index], keyHeight);
			}
		}
	}
}
Object.freeze(BottomKeyRowBehavior.prototype);

class KeyboardBehavior extends Behavior {
	onCreate(container, $, data) {
		container.skin = new Skin({ fill:Colors.BACKGROUND });
		let style = container.style = data.style;
		
		this.metrics = new Uint8Array(128);
		for (let i = 32; i < 128; i++)
			this.metrics[i] = style.measure(String.fromCharCode(i)).width;
		this.metrics.height = style.measure(String.fromCharCode(33)).height;
		
		this.ok = SUBMITDEFAULT;
		this.okMeasure = style.measure(this.ok);
		
		this.toggleMode = ToggleModes.NONE;
		this.expanded = false;
		this.expandState = 0;

		this.doTransition = data.doTransition;
		this.keyTarget = data.target;
		
		this.fadeKey = Int8Array.of(-1, -1);	// (index, row)
	}
	onDisplaying(container) {
		this.expansionStates = this.keyWidths.length;
		this.rowX = new Int16Array(this.expansionStates);
		this.rowX2 = new Int16Array(this.expansionStates);
		if (this.doTransition)
			this.startRowTransition(container, false);
	}
	onFinished(container) {
		delete this.timeline;
		container.bubble("onKeyboardTransitionFinished", this.transitionReverse);
	}
	onKeyboardOK(container, string) {
		if (this.doTransition)
			container.defer("startRowTransition", true);
	}
	onKeyUp(container, key) {
		let expand = false;
		let expanded = this.expanded;
		if (expanded && (key == '\r'))
			expand = true;
		else if (this.keyTarget)
			this.keyTarget.delegate("onKeyUp", key);
		if (key != '\b' && key != '\r' && key != ' ')
			expand = true;
		if (expand)
			container.defer("startRowExpansion")
		return true;
	}
	onTimeChanged(container) {
		let duration = container.duration;
		if (duration > 0) {
			let time = this.transitionReverse ? container.duration - container.time : container.time;
			this.timeline.seekTo(time);
		}
		else {
			++this.expandCount;
			if (this.expansionStates - 1 == this.expandCount) {
				if (this.transitionReverse) {
					this.expandState = 0;
					this.expanded = false;
					container.bubble("onKeyboardRowsContracted");
				}
				else {
					this.expanded = true;
					this.expandState = this.expansionStates - 1;
					container.bubble("onKeyboardRowsExpanded");
				}
				this.expandCount = -1;
				this.fadeKey[0] = -1;

				container.stop();
				//this.expandStop = Date.now();
				//trace(`expansion took ${this.expandStop - this.expandStart} ms, ${this.frames} frames, (${((this.frames) / 3 / (this.expandStop - this.expandStart) * 1000).toFixed(2)} fps)\n`);
				container.content(3).invalidate();
			}
			else {
				this.expandState = this.expandCount + 1;
				if (this.transitionReverse)
					this.expandState = this.expansionStates - 1 - this.expandState;
				container.content(0).invalidate();
				container.content(1).invalidate();
				container.content(2).invalidate();
			}
		}
	}
	onTouchBegan(port, id, x, y) {
		if (this.expanded) return;
		let content;
		if (y > port.content(2).y)
			content = port.content(2);
		else if (y > port.content(1).y)
			content = port.content(1);
		else
			content = port.content(0);
		this.tapTarget = content;
		content.delegate("onTouchBegan", id, x, content.y + 1);
	}
	onTouchEnded(port, id, x, y) {
		if (this.tapTarget) {
			this.tapTarget.delegate("onTouchEnded", id, x, this.tapTarget.y + 1);
			delete this.tapTarget;
		}
	}
	doSetTarget(container, keyTarget) {
		this.keyTarget = keyTarget;
	}
	startRowExpansion(container) {
		this.transitionReverse = this.expanded;
		this.expandCount = -1;
		container.interval = EXPANSION_INTERVAL;
		container.duration = -1;
		//this.expandStart = Date.now();
		//this.frames = 0;
		container.time = 0;
		container.start();
	}
	startRowTransition(container, out) {
		this.transitionReverse = out;

		let timeline = this.timeline = new Timeline;
		let height = container.height;
		let content;
		content = container.content(0);
		timeline.from(content, { y: content.y + height }, TIMELINE_DURATION, Math.quadEaseOut, 0);
		content = container.content(1);
		timeline.from(content, { y: content.y + height }, TIMELINE_DURATION, Math.quadEaseOut, TIMELINE_DELAY);
		content = container.content(2);
		timeline.from(content, { y: content.y + height }, TIMELINE_DURATION, Math.quadEaseOut, TIMELINE_DELAY);
		content = container.content(3);
		timeline.from(content, { y: content.y + height }, TIMELINE_DURATION, Math.quadEaseOut, TIMELINE_DELAY);
		container.duration = timeline.duration;
		container.time = 0;
		timeline.seekTo(out ? timeline.duration : 0);
		container.start();
 	}
}
Object.freeze(KeyboardBehavior.prototype);

class KeyboardFieldBehavior extends Behavior {
	onCreate(container, $, data) {
		container.duration = 500;
		this.string = data.string ?? "";
		this.password = data.password;
		container.last.skin = new Skin({ fill:container.style.color })
	}
	onDisplaying(container) {
		this.field = container.first;
		this.cursor = container.last;
		this.onKeyUp(container, "");
	}
	onFinished(container) {
		let field = this.field;
		let cursor = this.cursor;
		let style = container.style;
		let string = "*".repeat(this.string.length);
		cursor.x = field.x + style.measure(string).width + 2;
		field.string = string;
		cursor.start();
	}
	onKeyUp(container, key) {
		let field = this.field;
		let cursor = this.cursor;
		let password = this.password;
		let style = container.style;
		let width;
		if ('\r' == key) {
			application.distribute("onKeyboardOK", this.string);
			this.string = "";
		}
		else if ('\b' == key)
			this.string = this.string.slice(0, -1);
		else
			this.string += key;
		let length = this.string.length;
		if (password && length) {
			if ((key != '\b') && (key != '')) {
				cursor.stop();
				cursor.visible = false;
				field.string = "*".repeat(length - 1) + this.string.charAt(length - 1);
				container.time = 0;
				container.duration = 500;
				container.start();
			} else {
				let string = "*".repeat(length);
				width = style.measure(string).width;
				this.cursor.x = field.x + width + 2;
				field.string = string;
			}
			return;
		}
		width = style.measure(this.string).width;
		cursor.x = field.x + width + 2;
		field.string = this.string;
	}
}
Object.freeze(KeyboardFieldBehavior.prototype);

const KeyboardField = Container.template($ => ({
	Behavior:KeyboardFieldBehavior,
	contents: [
		Label($, { left:0, right:0, top:0, bottom:0 }),
		Content($, {
			left:0, width:1, top:5, bottom:5,
			visible:false,
			Behavior: class extends Behavior {
				onDisplaying(content) {
					content.interval = 500;
					content.start();
				}
				onTimeChanged(content) {
					content.visible = !content.visible;
				}
			}
		})
	]
}))

export {
	BottomKeyRowBehavior,
	Colors,
	ExpandingKeyRowBehavior,
	KeyboardBehavior,
	KeyboardField,
	KeyRowBehavior,
	Keys,
	ToggleModes,
}
