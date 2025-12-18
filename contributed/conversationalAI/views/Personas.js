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

class PersonasBehavior extends View.Behavior {
	onConfigure(container, persona) {
		if (controller.going)
			return;
		const view = this.view;
		const selection = view.personas.indexOf(persona);
		view.option = true;
		view.selection = selection;
		controller.goWith({
			View: "Options",
			persona
		});
	}
	onDisplaying(container) {
		const selection = this.view.selection;
		if (selection >= 0) {
			let row = container.last.first.content(selection);
			if (this.view.option)
				row = row.last.previous;
			row.behavior.changeState(row, 1);
		}
	}
	onSelect(container, persona) {
		if (controller.going)
			return;
		const view = this.view;
		const selection = view.personas.indexOf(persona);
		view.option = false;
		view.selection = selection;
		controller.goWith({
			View: "Home",
			...persona
		});
	}
}

class PersonaRowBehavior extends View.RowBehavior {
	onTap(container) {
		container.bubble("onSelect", this.data);
	}
}

class PersonaButtonBehavior extends View.RowBehavior {
	onTap(container) {
		container.bubble("onConfigure", this.data);
	}
}

class LevelsButtonBehavior extends View.RowBehavior {
	onTap(container) {
		controller.goTo("Levels");
	}
}

const PersonasContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:PersonasBehavior,
	contents: [
		$.constructor.StarsContainer($, {}),
		Container($, {
			left:0, width:240, top:0, height:38, clip:true,
			contents: [
				Label($, { string:"Assistants" }),
// 				Content($, { width:50, right:0, top:0, height:50, active:true, Behavior:LevelsButtonBehavior }),
			]
		}),
		Scroller($, {
			left:0, width:240, top:38, bottom:0, skin:assets.skins.personaRow, clip:true, active:true, backgroundTouch:true, Behavior:View.VerticalScrollerBehavior,
			contents: [
				Column($, { 
					left:0, right:0, top:0, 
					contents: $.personas.map($$ => new PersonaRow($$) ),
				}),
				View.VerticalScrollbar($, {}),
			]
		}),
	],
}));

const PersonaRow = Row.template($ => ({
	left:0, width:240, height:84, skin: assets.skins.personaRow, active:true, Behavior:PersonaRowBehavior,
	contents: [
		Content($, { left:8, width:24, skin:assets.services[$.service].iconSmall }),
		Column($, {
			left:8, right:8,
			contents: [
				Text($, { left:0, right:0, style:assets.styles.personaTitle, string:$.title }),
				Text($, { left:0, right:0, style:assets.styles.personaSubtitle, string:$.subtitle }),
			]
		}),
		Content($, { width:40, top:0, bottom:0, skin:assets.skins.gear, active:true, Behavior:PersonaButtonBehavior }),
		Content($, { width:20 }),
	]
}));

class PersonasTimeline extends Timeline {
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
		super(data);
		this.personas = controller.personas;
		this.option = false;
		this.selection = -1;
		this.scroll = { x:0, y:0 };
	}
	get Template() { return PersonasContainer }
	get Timeline() { return PersonasTimeline }
};
