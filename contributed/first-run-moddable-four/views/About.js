/*
 * Copyright (c) 2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import assets from "assets";
import Timeline from "piu/Timeline";
import View from "View";

const rowHeight = 28;

class AboutBehavior extends View.Behavior {
	onCreate(container, view) {
		super.onCreate(container, view);
		this.runningContents = [ null, null, null ];
		this.direction = 0;
		this.selection = 1;
		this.ticker = null;
	}
	onDisplayed(container) {
		this.onFinished(container);
	}
	onFinished(container) {
		if (this.selecting)
			this.selection += this.direction;
		this.direction = 0;
		this.ticker = container.hit(0, container.y + (this.selection * rowHeight));
		this.ticker?.delegate("onStart");
	}
	onJogDialTurned(container, delta) {
		if (container.running)
			return false;
		const view = this.view;
		const list = view.LIST;
		const scroller = list.container;
		const direction = -Math.sign(delta);
		this.scrolling = false;
		this.selecting = false;
		if (direction < 0) {
			if (this.selection == 1) {
				if (list.y < scroller.y) {
					this.scrolling = true;
					this.scrollY = scroller.scroll.y;
				}
			}
			else
				this.selecting = true;
		}
		else {
			if (this.selection == 3) {
				if (list.y + list.height > scroller.y + scroller.height) {
					this.scrolling = true;
					this.scrollY = scroller.scroll.y;
				}
			}
			else
				this.selecting = true;
		}
		this.direction = direction;
		container.duration = 200;
		container.time = 0;
		container.start();
		this.ticker?.delegate("onStop");
		return true;
	}
	onTimeChanged(container) {
		const delta = this.direction * Math.quadEaseOut(container.fraction);
		const selector = container.first;
		const scroller = selector.next;
		if (this.scrolling)
			scroller.scrollTo(0, this.scrollY + (delta * rowHeight));
		else if (this.selecting)
			selector.y = scroller.y + ((this.selection + delta) * rowHeight);
	}
	onUndisplaying(container) {
		this.runningContents.forEach(content => content?.delegate("onStop"));
	}
}

export class AboutColumnBehavior extends Behavior {
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
				holder.height = table.first.height;
			}
		}
		if (table)
			holder.first.y = holder.y - Math.max(0, holder.height - bottom + min);
	}
};

export class AboutHolderBehavior extends Behavior {
};

class AboutTableBehavior extends Behavior  {
	hold(column) {
		return AboutHeader(this.data.title, {left:0, right:0, top:0, height:column.first.height});
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
}

class AboutHeaderBehavior extends Behavior  {

}

class AboutTopArrowBehavior extends Behavior  {

}

class AboutBottomArrowBehavior extends Behavior  {
	onScrolled(container) {
		let scroller = container.container;
		let column = scroller.first;
		container.visible = (column.y + column.height > scroller.y + scroller.height);
	}
}

class AboutRowBehavior extends Behavior  {
	onCreate(row, $) {
	}
	onDisplaying(row, $) {
		row.interval = 20;
	}
	onStart(row) {
		row.start();
		return true;
	}
	onStop(row) {
		row.stop();
		row.first.scrollTo(0, 0);
	}
	onTimeChanged(row) {
		const scroller = row.first;
		const label = scroller.first;
		row.first.scrollBy(1, 0);
	}
}

class AboutQRCodeBehavior extends Behavior  {
	hold(column) {
	}
	onFlow(table, holder) {
	}
	onHold(table, holder) {
	}
}

const AboutContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:AboutBehavior,
	contents: [
		Content($, { anchor:"SELECTION", left: 0, width:128, top: rowHeight, height:rowHeight, skin:assets.skins.selection }),
		Scroller($, {
			left:0, width:128, top:0, height:128, clip:true,
			contents: [
				Column($, {
					anchor:"LIST", left:0, right:0, top:0, Behavior:AboutColumnBehavior, 
					contents: [
						$.items.map($$ => new AboutTable($$)),
						QRCode(undefined, { width:128, height:128, skin:assets.skins.qrcode, string:$.items.at(-1).url, Behavior:AboutQRCodeBehavior  }),
					]
				}),
				Container($, {
					left:0, right:0, top:0, height:rowHeight, clip:true, Behavior:AboutHolderBehavior,
				}),
				Content($, { top: -10, left: 2, visible:false, skin:assets.skins.arrows, Behavior:AboutTopArrowBehavior }),
				Container($, {
					left:0, right:0, height:16, bottom:0, skin:assets.skins.screen, Behavior:AboutBottomArrowBehavior,
					contents: [
						Content($, { bottom: 2, skin:assets.skins.arrows, variant:1 }),
					],
				}),
			]
		}),
	]
}));

const AboutHeader = Row.template($ => ({
	left:0, right:0, height:rowHeight, skin:assets.skins.focus, state:1, Behavior:AboutHeaderBehavior,
	contents: [
		Label($, { left:0, top:0, bottom:0, style:assets.styles.aboutHeader, state:1, string:$ }),
	]
}));

const AboutRow = Row.template($ => ({
	left:0, right:0, height:rowHeight, active:true, Behavior:AboutRowBehavior,
	contents: [
		Scroller($, {
			left:0, right:0, top:0, height:rowHeight, clip:true, looping:true,
			contents: [
				Label($, { left:0, top:0, bottom:0, style:assets.styles.aboutRow, string:$ }),
			]
		}),
	]
}));

var AboutTable = Column.template(function($) { return {
	left:0, right:0,
	Behavior: AboutTableBehavior,
	contents: [
		AboutHeader($.title, {}),
		$.items.map($$ => new AboutRow($$)),
	],
}});


class AboutTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super();
		const selector = screen.first;
		const scroller = selector.next;
		if (controller.going != direction) {
			this.from(scroller, { x:screen.x - screen.width }, 250, Math.quadEaseOut, 0);
			this.from(selector, { x:screen.x - screen.width }, 250, Math.quadEaseOut, -125);
		}
		else {
			this.from(scroller, { x:screen.x + screen.width }, 250, Math.quadEaseOut, 0);
			this.from(selector, { x:screen.x + screen.width }, 250, Math.quadEaseOut, -125);
		}
	}
}

export default class extends View {
	static get Behavior() { return AboutBehavior }
	
	constructor(data) {
		super(data);
		this.items = data.items;
	}
	get Template() { return AboutContainer }
	get Timeline() { return AboutTimeline }
};
