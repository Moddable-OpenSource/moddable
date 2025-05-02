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
	onDisplaying(container) {
		const selection = this.view.selection;
		if (selection >= 0) {
			const row = container.last.first.content(selection);
			row.behavior.changeState(row, 1);
		}
	}
	onSelect(container, persona) {
		if (controller.going)
			return;
		const selection = controller.model.indexOf(persona);
		this.view.selection = selection;
		controller.goWith({
			View: "Home",
			...persona
		});
	}
}

class PersonaBehavior extends Behavior {
	changeState(container, state) {
		container.state = state;
	}
	tweenState(container, from, to, duration) {
		this.from = from;
		this.to = to;
		container.duration = duration;
		container.time = 0;
		container.start();
	}
	onCreate(container, data, it) {
		this.data = data;
	}
	onDisplayed(container) {
		if (container.state == 1)
			this.tweenState(container, 1, 0, 250);
	}
	onTimeChanged(container) {
		this.changeState(container, this.from + (container.fraction * (this.to - this.from)));
	}
	onTouchBegan(container, id, x, y, ticks) {
		this.tweenState(container, 0, 1, 250);
	}
	onTouchCancelled(container) {
		container.stop();
		this.tweenState(container, container.fraction, 0, container.time);
	}
	onTouchEnded(container) {
		container.bubble("onSelect", this.data);
	}
	onUndisplaying(container) {
// 		container.active = false;
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
			]
		}),
		Scroller($, {
			left:0, width:240, top:38, bottom:0, skin:assets.skins.personaRow, clip:true, active:true, backgroundTouch:true, Behavior:View.VerticalScrollerBehavior,
			contents: [
				Column($, { 
					left:0, right:0, top:0, 
					contents: controller.model.map($$ => new PersonaContainer($$) ),
				}),
				View.VerticalScrollbar($, {}),
			]
		}),
	],
}));

const PersonaContainer = Container.template($ => ({
	left:0, width:240, height:84, skin: assets.skins.personaRow, active:true, Behavior:PersonaBehavior,
	contents: [
		Column($, {
			left:10, right:25, top:5,
			contents: [
				Text($, { left:0, right:0, style:assets.styles.personaTitle, string:$.title }),
				Label($, { left:0, right:0, style:assets.styles.personaSubtitle, string:$.subtitle }),
			]
		}),
		Content($, { right:28, bottom:10, skin:assets.services[$.service].iconSmall }),
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
		this.selection = -1;
		this.scroll = { x:0, y:0 };
	}
	get Template() { return PersonasContainer }
	get Timeline() { return PersonasTimeline }
};
