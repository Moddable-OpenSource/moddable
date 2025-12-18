/*
 * Copyright (c) 2024-2025 Moddable Tech, Inc.
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

import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";

class ModelsBehavior extends View.Behavior {
	onCreate(container, view) {
		super.onCreate(container, view);
		const column = view.LIST;
		const service = assets.services[view.persona.service];
		for (let provider of service.providers) {
			const table = new ProviderTable(provider)
			for (let model of provider.models) {
				table.add(new ModelRow(model));
			}
			column.add(table);
		}
	}
	onDisplaying(container) {
		const view = this.view;
		const persona = view.persona;
		container.distribute("onSelected", persona.providerID, persona.modelID, view.LIST.container);
	}
	onSelect(container, provider, model) {
		const view = this.view;
		const persona = view.persona;
		persona.providerID = provider.id;
		persona.modelID = model.id;
		controller.writeOption(persona);
		controller.goBack();
	}
}

class ModelsHolderBehavior extends Behavior {
	onCreate(table, view) {
		this.table = null;
		this.header = null;
	}
}

class ModelsColumnBehavior extends Behavior {
	onCreate(table, view) {
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
				holder.height = table.first.height /*+ table.last.height*/;
			}
		}
		if (table)
			holder.first.y = holder.y - Math.max(0, holder.height - bottom + min);
	}
}

class ProviderTableBehavior extends Behavior {
	onCreate(table, provider) {
		this.provider = provider;
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
		this.header = new ProviderHeader(this.provider, {left:0, right:0, top:0, height:table.first.height});
		this.header.behavior.held = true;
		this.header.behavior.table = table;
		holder.add(this.header);
	}
}

class ProviderHeaderBehavior extends Behavior {
	onCreate(row, provider) {
		this.held = false;
		this.table = null;
	}
}

class ModelRowBehavior extends View.RowBehavior {
	onCreate(row, model) {
		this.model = model;
	}
	onSelected(row, providerID, modelID, scroller) {
		const provider = row.container.behavior.provider;
		const model = this.model;
		if ((provider.id == providerID) && (model.id == modelID)) {
			row.last.previous.visible = true;
			if (scroller)
				scroller.reveal(row.bounds);
		}
		else
			row.last.previous.visible = false;
	}
	onTap(row) {
		row.bubble("onSelect", row.container.behavior.provider, this.model);
// 		this.tweenState(row, 1, 0, 250);
	}
}

const ModelsContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:ModelsBehavior,
	contents: [
		$.constructor.StarsContainer($, {}),
		Row($, {
			left:0, width:240, top:0, height:38, skin:assets.skins.homeTitle,
			contents: [
				Content($, { width:38, height:38, skin:assets.skins.back, active:true, Behavior:View.BackButtonBehavior }),
				Text($, { left:0, right:0, style:assets.styles.homeTitle, string:"Models" }),
			]
		}),
		Scroller($, {
			left:0, width:240, top:38, bottom:0, skin:assets.skins.personaRow, clip:true, active:true, backgroundTouch:true, Behavior:View.VerticalScrollerBehavior,
			contents: [
				Column($, { anchor:"LIST", left:0, right:0, top:0, Behavior:ModelsColumnBehavior }),
				Container($, { anchor:"HOLDER", left:0, right:0, top:0, height:30, Behavior:ModelsHolderBehavior }),
				View.VerticalScrollbar($, {}),
			]
		}),
	],
}));

const ProviderTable = Column.template($ => ({
	left:0, width:0, Behavior:ProviderTableBehavior,
	contents: [
		ProviderHeader($, {}),
	],
}));

const ProviderHeader = Row.template($ => ({
	left:0, width:240, height:30, skin: assets.skins.providerRow, Behavior:ProviderHeaderBehavior,
	contents: [
		Label($, { left:8, right:8, style:assets.styles.personaTitle, string:$.name }),
	],
}));

const ModelRow = Row.template($ => ({
	left:0, width:240, height:45, skin: assets.skins.personaRow, active:true, Behavior:ModelRowBehavior,
	contents: [
		Content($, { width:16 }),
		Label($, { left:8, right:8, style:assets.styles.personaSubtitle, string:$.name }),
		Content($, { width:24, top:0, bottom:0, skin:assets.skins.check, visible:false }),
		Content($, { width:20 }),
	]
}));

class ModelsTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		let header = screen.first.next;
		let body = screen.last;
		const duration = 250;
		this.from(header, { y:screen.y - header.height }, duration, Math.quadEaseOut, 0);
		if (controller.going != direction)
			this.from(body, { x:screen.x - body.width }, duration, Math.quadEaseOut, -duration);
		else
			this.from(body, { x:screen.x + body.width }, duration, Math.quadEaseOut, -duration);
	}
}

export default class extends View {
	constructor(data) {
		super();
		this.persona = data.persona;
		const service = this.persona.service;
		this.providers = this.persona.service.providers
		
		this.names = [];
		for (let name in assets.services) {
			if (name == service)
				this.selection = this.names.length;
			this.names.push(name);
		}
		this.scroll = { x:0, y:0 };
	}
	get Template() { return ModelsContainer }
	get Timeline() { return ModelsTimeline }
};
