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


// ASSETS

import {
	headerHeight,
	rowHeight,
	rowIndent,
} from "assets";

// BEHAVIORS

import { 
	ButtonBehavior, 
	ScrollerBehavior,
	HolderColumnBehavior,
	HolderContainerBehavior,
	RowBehavior,
	HeaderBehavior,
	TableBehavior,
	SpinnerBehavior,
} from "behaviors";

class MessagePaneBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
		container.distribute("onConversationsChanged", data.conversations.items);
		container.distribute("onBubblesChanged", data.bubbles.items);
	}
};

class ConversationTableBehavior extends TableBehavior {
	expand(column, expandIt) {
		var data = this.data;
		var header = column.first;
		data.expanded = expandIt;
		column.empty(1);
		if (expandIt) {
			header.behavior.expand(header, true);
			for (let item of data.items)
				column.add(new ConversationRow(item));
			column.add(new ConversationFooter(data));
		}
		else {
			header.behavior.expand(header, false);
		}
	}
	hold(column) {
		return ConversationHeader(this.data, {left:0, right:0, top:0, height:column.first.height});
	}
	onCreate(column, data) {
		this.data = data;
		this.expand(column, data.expanded);
	}
	onConversationsChanged(column, items) {
		var data = this.data;
		this.expand(column, data.expanded);
	}
}

class ConversationHeaderBehavior extends HeaderBehavior {
	reveal(row, revealIt) {
		row.last.visible = revealIt;
	}
};

class ConversationRowBehavior extends RowBehavior {
	onTap(row) {
		row.bubble("doToggleConversation", this.data);
	}
};

class BubbleTableBehavior extends Behavior {
	onBubblesChanged(column, items) {
		let scroller = column.container;
		let flag = scroller.scroll.y >= (column.height - scroller.height);
		let i = 0;
		let c = items.length;
		column.empty();
		while (i < c) {
			let item = items[i];
			if (item.conversation.visible) {
				if (item.flags & 4) {
					if (item.flags & 2)
						column.add(new BubbleRightTextRow(item));
					else if (item.flags & 1)
						column.add(new BubbleLeftTextRow(item));
					else
						column.add(new BubbleCenterTextRow(item));
				}
				else {
					if (item.flags & 2)
						column.add(new BubbleRightCodeRow(item));
					else if (item.flags & 1)
						column.add(new BubbleLeftCodeRow(item));
					else
						column.add(new BubbleCenterCodeRow(item));
				}
			}
			i++;
		}
		if (flag)
			scroller.scrollTo(0, 0x7FFFFFFF);
	}
	onCreate(column, data) {
		this.data = data;
	}
	onDisplaying(column) {
		let scroller = column.container;
		scroller.scrollTo(0, 0x7FFFFFFF);
	}
}

class BubbleCodeRowBehavior extends RowBehavior {
	onMeasureHorizontally(layout, width) {
		return Math.min(width, model.FEATURE.width - 40);
	}
	onTap(row) {
		let data = this.data;
		model.selectFile(data.path, { line:data.line });
	}
}

class BubbleTextRowBehavior extends RowBehavior {
	onCreate(layout, data) {
		this.data = data;
		let size = styles.message.measure("_");
		this.width = size.width;
	}
	onMeasureHorizontally(layout, width) {
		width = this.width * this.data.message.length;
		return Math.min(width + 30, model.FEATURE.width - 40);
	}
	onTap(row) {
		let data = this.data;
		model.selectFile(data.path, { line:data.line });
	}
}

// TEMPLATES

import {
	Code
}
from "piu/Code";

import {
	VerticalScrollbar,
} from "piu/Scrollbars";

export var MessagePane = Container.template(function($) { return {
	left:0, right:0, top:0, bottom:0, skin:skins.messageBackground,
	Behavior: MessagePaneBehavior,
	contents: [
		Column($, {
			left:0, right:0, top:0, bottom:0, active:true, clip:true, 
			contents: [
				ConversationTable($.conversations, {}),
				Scroller($, {
					left:0, right:0, top:0, bottom:0, active:true, clip:true, Behavior:ScrollerBehavior, 
					contents: [
						BubbleTable($.bubbles, {}),
						VerticalScrollbar($, {}),
					]
				}),
			]
		}),
	
	]
}});

var ConversationTable = Column.template($ => ({
	left:0, right:0, active:true, 
	Behavior:ConversationTableBehavior,
	contents: [
		ConversationHeader($, { name:"HEADER" }),
	],
}));

var ConversationHeader = Row.template(function($) { return {
	left:0, right:0, height:headerHeight, skin:skins.tableHeader, active:true,
	Behavior: ConversationHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:26, top:5, skin:skins.glyphs, variant:1 }),
		Label($, { left:0, right:0, style:styles.tableHeader, string:"MESSAGES" }),
		IconButton($, { top:0, variant:5, active:true, visible:false, 
			Behavior: class extends ButtonBehavior {
				onBubblesChanged(button) {
					button.active = model.canClearAllBubbles();
					this.changeState(button, button.active ? 1 : 0);
				}
				onTap(button) {
					model.doClearAllBubbles();
				}
			},
		}),
	],
}});

var ConversationFooter = Row.template(function($) { return {
	left:0, right:0, height:3, skin:skins.tableFooter,
}});

var ConversationRow = Row.template(function($) { return {
	left:0, right:0, height:headerHeight, skin:skins.tableRow, style:styles.conversation, active:true, 
	Behavior:ConversationRowBehavior,
	contents: [
		Content($, { width:rowIndent, }),
		Content($, { width:rowIndent, skin:skins.conversation, variant:$.visible ? 1 : 0 }),
		Container($, { clip:true,
			contents:[
				RoundContent($, { left:4, right:4, top:2, bottom:2, radius:8, border:1, skin:skins.messages[$.tint] }),
				Label($, { string:$.id || "anonymous" }),
			]
		}),
	]
}});

var BubbleTable = Column.template($ => ({
	left:0, right:0, top:0, active:false, style:styles.message,
	Behavior:BubbleTableBehavior,
	contents: [
	],
}));

var BubbleCenterCodeRow = Layout.template($ => ({
	clip:true, active:true,
	Behavior: BubbleCodeRowBehavior,
	contents: [
		RoundContent($, { left:4, right:4, top:2, bottom:2, radius:8, border:1, skin:skins.messages[$.conversation.tint] }),
		Container($, { left:12, right:12, clip:true,
			contents:[
				Code($, { left:0, top:0, string:$.message, type:"json", active:false }),
			]
		}),
	],
}));

var BubbleLeftCodeRow = Layout.template($ => ({
	left:0, clip:true, active:true,
	Behavior: BubbleCodeRowBehavior,
	contents: [
		RoundContent($, { left:4, right:4, top:2, bottom:2, radius:8, border:1, skin:skins.messages[$.conversation.tint], variant:1 } ), 
		Container($, { left:20, right:12, clip:true,
			contents:[
				Code($, { left:0, top:0, string:$.message, type:"json", active:false }),
			]
		}),
	],
}));

var BubbleRightCodeRow = Layout.template($ => ({
	right:0, clip:true, active:true,
	Behavior: BubbleCodeRowBehavior,
	contents: [
		RoundContent($, { left:4, right:4, top:2, bottom:2, radius:8, border:1, skin:skins.messages[$.conversation.tint], variant:2 } ), 
		Container($, { left:12, right:20, clip:true,
			contents:[
				Code($, { left:0, top:0, string:$.message, type:"json", active:false }),
			]
		}),
	],
}));

var BubbleCenterTextRow = Layout.template($ => ({
	clip:true, active:true,
	Behavior: BubbleTextRowBehavior,
	contents: [
		RoundContent($, { left:4, right:4, top:2, bottom:2, radius:8, border:1, skin:skins.messages[$.conversation.tint] }), 
		Column($, { left:12, right:12, clip:true,
			contents:[
				Content($, { height:4 }),
				Text($, { left:0, right:0, top:0, string:$.message }),
				Content($, { height:4 }),
			]
		}),
	],
}));

var BubbleLeftTextRow = Layout.template($ => ({
	left:0, clip:true, active:true,
	Behavior: BubbleTextRowBehavior,
	contents: [
		RoundContent($, { left:4, right:4, top:2, bottom:2, radius:8, border:1, skin:skins.messages[$.conversation.tint], variant:1 } ), 
		Column($, { left:20, right:12, clip:true,
			contents:[
				Content($, { height:4 }),
				Text($, { left:0, right:0, top:0, string:$.message }),
				Content($, { height:4 }),
			]
		}),
	],
}));

var BubbleRightTextRow = Layout.template($ => ({
	right:0, clip:true, active:true,
	Behavior: BubbleTextRowBehavior,
	contents: [
		RoundContent($, { left:4, right:4, top:2, bottom:2, radius:8, border:1, skin:skins.messages[$.conversation.tint], variant:2 } ), 
		Column($, { left:12, right:20, clip:true,
			contents:[
				Content($, { height:4 }),
				Text($, { left:0, right:0, top:0, string:$.message }),
				Content($, { height:4 }),
			]
		}),
	],
}));
