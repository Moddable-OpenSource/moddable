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

class AsleepBehavior extends View.Behavior {
	onCreate(container, view) {
		super.onCreate(container, view);
	}
	onDisplayed(container) {
		if (this.view.wakenWith == null) {
			controller.view = controller.history.pop();
			controller.sleep({ button:true, jogdial:true }, 666);
		}
		else {
			controller.goBack();
		}
	}
}

const AsleepContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:AsleepBehavior,
	contents: [
		Container($, {
			left:0, width:128, top:0, height:64, visible:true,
			contents: [
				Label($, {left:0, width:128, top:4, height:26, style:assets.styles.wake, string:"Moddable Four" }),
				Label($, {left:0, width:128, top:30, height:26, string:"is asleep" }),
			], 
		}),
		Container($, {
			left:0, width:128, top:64, height:64, skin:assets.skins.focus, state:1, visible:true,
			contents: [
				Label($, {left:0, width:128, top:4, height:26, state:1, string:"Press button" }),
				Label($, {left:0, width:128, top:30, height:26, state:1, string:"to wake" }),
			], 
		}),
	]
}));

class AsleepTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		let content = screen.first;
		this.from(content, { x:screen.x + screen.width }, 250, Math.quadEaseOut, 0);
		this.from(content.next, { x:screen.x + screen.width }, 250, Math.quadEaseOut, -125);
	}
}

export default class extends View {
	static get Behavior() { return AsleepBehavior }
	
	constructor(data) {
		super(data);
		this.wakenWith = null;
	}
	get Template() { return AsleepContainer }
	get Timeline() { return AsleepTimeline }
};
