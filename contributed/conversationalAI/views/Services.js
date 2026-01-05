/*
 * Copyright (c) 2024-2026 Moddable Tech, Inc.
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

class ServicesBehavior extends View.Behavior {
	onDisplaying(/* container */) {
		const view = this.view;
		const row = view.LIST.content(view.selection);
		row.last.previous.visible = true;
	}
	onSelect(container, current) {
		const view = this.view;
		const former = view.LIST.content(view.selection);
		former.last.previous.visible = false;
		current.last.previous.visible = true;
		const name = view.names[current.index];
		const persona = view.persona;
		const service = assets.services[name];
		if (persona.service != name) {
			persona.service = name
			persona.voiceID = service.defaultVoiceID;
			persona.providerID = service.defaultProviderID;
			persona.modelID = service.defaultModelID;
			controller.writeOption(persona);
		}
		controller.goBack();
	}
}

class ServiceRowBehavior extends View.RowBehavior {
	onTap(row) {
		row.bubble("onSelect", row);
// 		this.tweenState(row, 1, 0, 250);
	}
}

const ServicesContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:ServicesBehavior,
	contents: [
		$.constructor.StarsContainer($, {}),
		Row($, {
			left:0, width:240, top:0, height:38, skin:assets.skins.homeTitle,
			contents: [
				Content($, { width:38, height:38, skin:assets.skins.back, active:true, Behavior:View.BackButtonBehavior }),
				Text($, { left:0, right:0, style:assets.styles.homeTitle, string:"Services" }),
			]
		}),
		Scroller($, {
			left:0, width:240, top:38, bottom:0, skin:assets.skins.personaRow, clip:true, active:true, backgroundTouch:true, Behavior:View.VerticalScrollerBehavior,
			contents: [
				Column($, { 
					anchor:"LIST", left:0, right:0, top:0, 
					contents: $.names.map($$ => {
						const service = assets.services[$$]; 
						if (service.key)
							return new ServiceRow(service) 
						return new NoServiceRow(service) 
					}),
				}),
				View.VerticalScrollbar($, {}),
			]
		}),
	],
}));

const ServiceRow = Row.template($ => ({
	left:0, width:240, height:45, skin: assets.skins.personaRow, active:true, Behavior:ServiceRowBehavior,
	contents: [
		Content($, { left:8, width:24, skin:$.iconSmall }),
		Label($, { left:8, right:8, style:assets.styles.personaTitle, string:$.title }),
		Content($, { width:24, top:0, bottom:0, skin:assets.skins.check, visible:false }),
		Content($, { width:20 }),
	]
}));

const NoServiceRow = Row.template($ => ({
	left:0, width:240, height:65, skin: assets.skins.personaRow, active:true, Behavior:ServiceRowBehavior,
	contents: [
		Content($, { left:8, width:24, skin:$.iconSmall }),
		Column($, {
			left:8, right:8, 
			contents: [
				Label($, { left:0, right:0, style:assets.styles.personaTitle, string:$.title }),
				Label($, { left:0, right:0, style:assets.styles.personaSubtitle, state:1, string:"No API key" }),
			]
		}),
		Content($, { width:24, top:0, bottom:0, skin:assets.skins.check, visible:false }),
		Content($, { width:20 }),
	]
}));

class ServicesTimeline extends Timeline {
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
		this.names = [];
		for (let name in assets.services) {
			if (name == service)
				this.selection = this.names.length;
			this.names.push(name);
		}
		this.scroll = { x:0, y:0 };
	}
	get Template() { return ServicesContainer }
	get Timeline() { return ServicesTimeline }
};
