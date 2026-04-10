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

class VoicesBehavior extends View.Behavior {
	onDisplaying(container) {
		const view = this.view;
		if (view.selection >= 0) {
			const row = view.LIST.content(view.selection);
			row.first.next.visible = true;
			view.LIST.container.reveal(row.bounds);
		}
	}
	onSelect(container, current) {
		const view = this.view;
		if (view.selection >= 0) {
			const former = view.LIST.content(view.selection);
			former.first.next.visible = false;
		}
		current.first.next.visible = true;
		view.persona.voiceID = view.voices[current.index].id;
		controller.writeOption(view.persona);
		controller.goBack();
	}
}

class VoiceRowBehavior extends View.RowBehavior {
	onCreate(row, data) {
		super.onCreate(row, data);
		function add(it, state) {
			if (it) {
				if (Array.isArray(it)) {
					it.forEach(item => add(item, state));
					return;
				}
				const label = new VoiceLabel(it);
				label.state = state;
				row.add(label);
			}
		}
		add(data.gender, 0);
		add(data.age, 1);
		add(data.accent, 1);
		add(data.description, 2);
		add(data.usage, 3);
	}
	onFitHorizontally(row, rowWidth) {
		rowWidth -= 20;
		let label = row.first.next.next;
		let coordinates = { left:4, top:40, width:0, height:20 };
		while (label) {
			const size = label.measure();
			coordinates.width = size.width;
			if (coordinates.left + coordinates.width + 4 > rowWidth) {
				coordinates.left = 4
				coordinates.top += coordinates.height + 4
			}
			label.coordinates = coordinates;
			coordinates.left += coordinates.width + 4;
			label = label.next;
		}
		if (coordinates.left > 4)
			coordinates.top += coordinates.height + 4
		this.rowHeight = coordinates.top;
	}
	onMeasureVertically(container, gridHeight) {
		return this.rowHeight;
	}
	onTap(row) {
		row.bubble("onSelect", row);
// 		this.tweenState(row, 1, 0, 250);
	}
}

const VoicesContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:VoicesBehavior,
	contents: [
		$.constructor.StarsContainer($, {}),
		Row($, {
			left:screen.corner, width:screen.width - (screen.corner << 1), top:0, height:38, skin:assets.skins.homeTitle,
			contents: [
				Content($, { width:38, height:38, skin:assets.skins.back, active:true, Behavior:View.BackButtonBehavior }),
				Text($, { left:0, right:0, style:assets.styles.homeTitle, string:"Voices" }),
				Content($, { width:38, height:38, skin:assets.services[$.persona.service].iconSmall }),
			]
		}),
		Scroller($, {
			left:0, width:screen.width, top:38, bottom:0, skin:assets.skins.personaRow, clip:true, active:true, backgroundTouch:true, Behavior:View.VerticalScrollerBehavior,
			contents: [
				Column($, { 
					anchor:"LIST", left:0, right:0, top:0, 
					contents: $.voices.map($$ => new VoiceRow($$) ),
				}),
				View.VerticalScrollbar($, {}),
			]
		}),
	],
}));

const VoiceRow = Layout.template($ => ({
	left:0, width:screen.width, skin: assets.skins.personaRow, active:true, Behavior:VoiceRowBehavior,
	contents: [
		Label($, { left:8, right:44, top:12, style:assets.styles.personaTitle, string:$.name }),
		Content($, { width:24, right:20, top:10, height:20, skin:assets.skins.check, visible:false }),
	],
}));

const VoiceLabel = Label.template($ => ({
	skin:assets.skins.voiceLabel, style:assets.styles.voiceLabel, string:$
}));

class VoicesTimeline extends Timeline {
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
		const persona = data.persona
		const voices = assets.services[persona.service].voices;
		const voiceID = persona.voiceID;
		this.persona = persona;
		this.selection = voices.findIndex(voice => voice.id == voiceID);
		this.scroll = { x:0, y:0 };
		this.voices = voices;
	}
	get Template() { return VoicesContainer }
	get Timeline() { return VoicesTimeline }
};
