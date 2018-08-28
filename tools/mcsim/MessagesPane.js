/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
 */

// ASSETS

import {
	buttonsSkin,
	glyphsSkin,
	paneBodySkin,
	paneBorderSkin,
	paneHeaderSkin,
	paneHeaderStyle,
} from "assets";	

var messageTexture = new Texture({ path:"assets/bubbles.png" });
var messageOutSkin = new Skin({ texture: messageTexture, x:0, y:0, width:100, height:64, tiles:{ left:20, right:20, top:20, bottom:20 }, states:64 });
var messageInSkin = new Skin({ texture: messageTexture, x:100, y:0, width:100, height:64, tiles:{ left:20, right:20, top:20, bottom:20 }, states:64 });
var messageListSkin = new Skin({ fill:"#e2e2e2" });
const CODE_BLACK = "#000000";
const CODE_COMMENT = "#008d32";
const CODE_KEYWORD = "#103ffb";
const CODE_LITERAL = "#b22821";
var messageListStyle = new Style({ font:"11px Fira Mono", color:[ CODE_BLACK, CODE_KEYWORD, CODE_LITERAL, CODE_COMMENT ], top:5, bottom:5, horizontal:"left" });

// BEHAVIORS

import {
	ButtonBehavior
} from "piu/Buttons";

class MessagePaneBehavior extends Behavior {
	add(container, scroller, line) {
		var column = scroller.first;
		var flag = scroller.scroll.y >= (column.height - scroller.height);
		if (column.length == 1024)
			column.remove(column.first);
		column.add(line);
		if (flag)
			scroller.scrollTo(0, 0x7FFFFFFF);
	}
	doCopy(container) {
		let content = container.first.first.first;
		let string = "";
		while (content) {
			string += content.first.first.string;
			string += "\n\n";
			content = content.next;
		}
		system.setClipboardString(string);
	}
	formatBuffer(buffer) {
		let array = new Uint8Array(buffer);
		let string = "";
		array.forEach(value => {
			if (value < 16)
				string += "0";
			string += value.toString(16);
			string += " ";
		});
		return string;
	}
	formatMessage(message) {
		return JSON.stringify(message, null, 2);
	}
	input(container, message, buffer, state = 0) {
		var scroller = container.first;
		if (message)
			message = this.formatMessage(message);
		else
			message = "...";
		if (buffer)
			buffer = this.formatBuffer(buffer);
		else
			buffer = "...";
		this.add(container, scroller, new MessageInLine(message, { state }));
		this.add(container, scroller.next, new BufferInLine(buffer, { state }));
	}
	output(container, message, buffer, state = 0) {
		var scroller = container.first;
		if (message)
			message = this.formatMessage(message);
		else
			message = "...";
		if (buffer)
			buffer = this.formatBuffer(buffer);
		else
			buffer = "...";
		this.add(container, scroller, new MessageOutLine(message, { state }));
		this.add(container, scroller.next, new BufferOutLine(buffer, { state }));
	}
	onEmptyMessages(container) {
		var scroller = container.first;
		scroller.first.empty();
		scroller.next.first.empty();
	}
	onMessagesKindChanged(container) {
		var scroller = container.first;
		scroller.visible = scroller.active = !model.messagesKind;
		scroller = scroller.next;
		scroller.visible = scroller.active = model.messagesKind;
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
		button.previous.visible = flag;
	}
}

class MessageHeaderBehavior extends Behavior {
	changeArrowState(row, state) {
		row.first.state = state;
	}
	onDisplaying(row) {
		this.changeArrowState(row, (model.orientation) ? 3 : 1);
	}
	onMouseEntered(row, x, y) {
		row.state = 1;
	}
	onMouseExited(row, x, y) {
		row.state = 0;
	}
	onTouchBegan(row) {
		row.state = 2;
		this.changeArrowState(row, 2);
	}
	onTouchEnded(row) {
		row.state = 1;
		application.delegate("onToggleMessages");
	}
}

class MessageLineBehavior extends Behavior {
	onMeasureHorizontally(layout, width) {
		return Math.min(width, model.VERTICAL_DIVIDER.previous.previous.width - 40);
	}
}

class BufferLineBehavior extends Behavior {
	onCreate(layout) {
		let size = messageListStyle.measure("_");
		this.width = size.width;
	}
	onMeasureHorizontally(layout, width) {
		width = this.width * layout.first.first.string.length;
		return Math.min(width + 30, model.VERTICAL_DIVIDER.previous.previous.width - 40);
	}
}

// TEMPLATES

import {
	Code
} from "piu/Code";

import {
	ScrollerBehavior,
	VerticalScrollbar
} from "piu/Scrollbars";

var MessageInLine = Layout.template($ => ({
	right:0, skin:messageInSkin, clip:true,
	Behavior: MessageLineBehavior,
	contents: [
		Container($, { left:15, right:15, clip:true,
			contents:[
				Code($, { left:0, top:0, string:$, type:"json" }),
			]
		}),
	],
}));

var MessageOutLine = Layout.template($ => ({
	left:0, skin:messageOutSkin, clip:true,
	Behavior: MessageLineBehavior,
	contents: [
		Container($, { left:15, right:15, clip:true,
			contents:[
				Code($, { left:0, top:0, string:$, type:"json" }),
			]
		}),
	],
}));

var BufferInLine = Layout.template($ => ({
	right:0, skin:messageInSkin, clip:true,
	Behavior: BufferLineBehavior,
	contents: [
		Container($, { left:15, right:15, clip:true,
			contents:[
				Text($, { left:0, right:0, top:0, string:$ }),
			]
		}),
	],
}));

var BufferOutLine = Layout.template($ => ({
	left:0, skin:messageOutSkin, clip:true,
	Behavior: BufferLineBehavior,
	contents: [
		Container($, { left:15, right:15, clip:true,
			contents:[
				Text($, { left:0, right:0, top:0, string:$ }),
			]
		}),
	],
}));

var MessagesPane = Container.template($ =>  ({
	left:0, right:0, top:0, bottom:0, Behavior: MessagePaneBehavior,
	contents: [
		Scroller($, {
			left:0, right:0, top:27, bottom:0, clip:true, skin:paneBodySkin, active:!$.messagesKind, visible:!$.messagesKind, Behavior:ScrollerBehavior,
			contents: [
				Column($, {
					left:0, right:0, top:0, style:messageListStyle,
				}),
				VerticalScrollbar($, {}),
			],
		}),
		Scroller($, {
			left:0, right:0, top:27, bottom:0, clip:true, skin:paneBodySkin, active:$.messagesKind, visible:$.messagesKind, Behavior:ScrollerBehavior,
			contents: [
				Column($, {
					left:0, right:0, top:0, style:messageListStyle,
				}),
				VerticalScrollbar($, {}),
			],
		}),
		Content($, { left:0, right:0, top:26, height:1, skin:paneBorderSkin, }),
		Row($, {
			left:0, right:0, top:0, height:26, skin:paneHeaderSkin, active:true, Behavior:MessageHeaderBehavior,
			contents: [
				Content($, { width:30, height:26, skin:glyphsSkin, variant:0 }),
				Label($, { left:0, right:0, style:paneHeaderStyle, string:"MESSAGES", state:1 }),
				Content($, {
					width:30, skin:buttonsSkin, variant:$.messagesKind ? 2 : 1, active:true, visible:false, 
					Behavior: class extends ButtonBehavior {
						onMessagesKindChanged(button) {
							button.variant = model.messagesKind ? 2 : 1;
						}
						onTap(label) {
							model.messagesKind = !model.messagesKind;
							model.MESSAGES.distribute("onMessagesKindChanged");
						}
					},
				}),
				Content($, {
					width:30, skin:buttonsSkin, variant:0, active:true, visible:false, 
					Behavior: class extends ButtonBehavior {
						onTap(button) {
							button.bubble("onEmptyMessages");
						}
					},
				}),
			],
		}),
	],
}));

export default MessagesPane;
