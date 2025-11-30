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

class OptionsBehavior extends View.Behavior {
	onDisplaying(container) {
		const selection = this.view.selection;
		if (selection >= 0) {
			const row = container.last.first.content(selection);
			row.behavior.changeState(row, 1);
		}
	}
	onSelectService(container) {
		const view = this.view;
		view.selection = 0;
		controller.goWith({
			View: "Services",
			persona: view.persona,
		});
	}
	onSelectVoice(container) {
		const view = this.view;
		view.selection = 1;
		controller.goWith({
			View: "Voices",
			persona: view.persona,
		});
	}
	onSelectModel(container) {
		const view = this.view;
		view.selection = 2;
		controller.goWith({
			View: "Models",
			persona: view.persona,
		});
	}
}

const OptionsContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:OptionsBehavior,
	contents: [
		$.constructor.StarsContainer($, {}),
		Row($, {
			left:0, width:240, top:0, height:38, skin:assets.skins.homeTitle,
			contents: [
				Content($, { width:38, height:38, skin:assets.skins.back, active:true, Behavior:View.BackButtonBehavior }),
				Text($, { left:0, right:0, style:assets.styles.homeTitle, string:$.persona.title }),
			]
		}),
		Scroller($, {
			left:0, width:240, top:38, bottom:0, skin:assets.skins.personaRow, clip:true, active:true, backgroundTouch:true, Behavior:View.VerticalScrollerBehavior,
			contents: [
				Column($, { 
					left:0, right:0, top:0, 
					contents: [
						Row($, {
							left:0, right:0, height:64, skin:assets.skins.personaRow, active:true,
							Behavior: class extends View.RowBehavior {
								onTap(row) {
									row.bubble("onSelectService");
								}
							},
							contents: [
								Content($, { left:8, width:24, skin:assets.services[$.persona.service].iconSmall }),
								Column($, {
									left:8, right:8,
									contents: [
										Label($, { left:0, right:0, style:assets.styles.personaSubtitle, string:"Service" }),
										Content($, { height:4 }),
										Label($, { left:0, right:0, style:assets.styles.personaTitle, string:assets.services[$.persona.service].title }),
									]
								}),
								Content($, { left:8, width:24, skin:assets.skins.forward }),
							]
						}),
						Row($, {
							left:0, right:0, height:64, skin:assets.skins.personaRow, active:true,
							Behavior: class extends View.RowBehavior {
								onCreate(row, $) {
									super.onCreate(row, $);
									const service = assets.services[$.persona.service];
									const voiceID = $.persona.voiceID;
									const voice = service.voices.find(voice => voice.id == voiceID);
									row.last.previous.last.string = voice.name;
								}
								onTap(row) {
									row.bubble("onSelectVoice");
								}
							},
							contents: [
								Content($, { left:8, width:24 }),
								Column($, {
									left:8, right:8,
									contents: [
										Label($, { left:0, right:0, style:assets.styles.personaSubtitle, string:"Voice" }),
										Content($, { height:4 }),
										Label($, { left:0, right:0, style:assets.styles.personaTitle }),
									]
								}),
								Content($, { left:8, width:24, skin:assets.skins.forward }),
							]
						}),
// 						Row($, {
// 							left:0, right:0, height:80, skin:assets.skins.personaRow, active:true,
// 							Behavior: class extends View.RowBehavior {
// 								onCreate(row, $) {
// 									super.onCreate(row, $);
// 									const service = assets.services[$.persona.service];
// 									const providers = service.providers;
// 									if (!providers) {
// 										row.active = row.visible = false;
// 										return;
// 									}
// 									const providerID = $.persona.providerID;
// 									const modelID = $.persona.modelID;
// 									const provider = providers.find(provider => provider.id == providerID);
// 									const model = provider.models.find(model => model.id == modelID);
// 									const column = row.last.previous;
// 									const label = column.first.next.next;
// 									label.string = provider.name;
// 									label.next.string = model.name;
// 								}
// 								onTap(row) {
// 									row.bubble("onSelectModel");
// 								}
// 							},
// 							contents: [
// 								Content($, { left:8, width:24 }),
// 								Column($, {
// 									left:8, right:8,
// 									contents: [
// 										Label($, { left:0, right:0, style:assets.styles.personaSubtitle, string:"Model" }),
// 										Content($, { height:4 }),
// 										Label($, { left:0, right:0, style:assets.styles.personaTitle }),
// 										Label($, { left:0, right:0, style:assets.styles.personaTitle }),
// 									]
// 								}),
// 								Content($, { left:8, width:24, skin:assets.skins.forward }),
// 							]
// 						}),
					]
				}),
				View.VerticalScrollbar($, {}),
			]
		}),
	],
}));

class OptionsTimeline extends Timeline {
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
		this.selection = -1;
		this.scroll = { x:0, y:0 };
	}
	get Template() { return OptionsContainer }
	get Timeline() { return OptionsTimeline }
};
