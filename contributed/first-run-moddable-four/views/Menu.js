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

class MenuBehavior extends View.Behavior {
	addMenuItem(container, view) {
		let item = view.items[view.index];
		let MenuItem = view.constructor[item.View];
		if (!MenuItem)
			MenuItem = MenuItemContainer;
		container.add(new MenuItem(item));
	}
	onCreate(container, view) {
		super.onCreate(container, view);
		this.direction = 0;
		this.half = Math.PI / 2;
		this.addMenuItem(container, view);
	}
	onDisplaying(container, view) {
		this.height = container.height;
		this.top = container.y;
		this.width = container.width;
		this.right = container.x + container.width;
	}
	onFinished(container) {
		container.remove(container.last.previous);
		controller.onJogDialTurned(application, 0);
	}
	onJogDialReleased(container) {
		const view = this.view;
		const item = view.items[view.index];
		controller.goWith(item);
		return true;
	}
	onJogDialTurned(container, delta) {
		if (container.running)
			return false;
		const view = this.view;
		let index = view.index;
		let direction = Math.sign(delta);
		delta = Math.abs(delta);
		index += direction * Math.round(delta / 4);
		index %= view.count;
		if (index < 0)
			index = view.count + index;
		this.direction = direction;
		view.index = index;
		
		this.addMenuItem(container, view);
		
		container.time = 0;
		container.duration = 350;
		container.start();
		return true;
	}
	onTimeChanged(container) {
		const current = container.last;
		const former = current.previous;
		const fraction = Math.quadEaseOut(container.fraction);
		const { half, right, width, top, height } = this;
		const dao = fraction * half;
		const dai = (1 - fraction) * half;
		if (this.direction > 0) {
			former.x = right - (width * Math.cos(dao));
			former.y = top - (height * Math.sin(dao));
			current.x = right - (width * Math.cos(dai));
			current.y = top + (height * Math.sin(dai));
		}
		else {
			former.x = right - (width * Math.cos(dao));
			former.y = top + (height * Math.sin(dao));
			current.x = right - (width * Math.cos(dai));
			current.y = top - (height * Math.sin(dai));
		}
	}
}

const MenuItemContainer = Container.template($ => ({
	left:0, width:128, top:0, height:128, clip:true,
	contents: [
		Container($, {
			left:0, right:0, top:12, height:72,
			contents: [
				Content($, { skin:assets.skins[$.icon] }),
			]
		}),
		Label($, { left:0, right:0, height:24, bottom:12, string:$.label }),
	]
}));

const MenuContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:$.constructor.Behavior,
	contents: [
		Content($, { top: 0, left: 2, skin:assets.skins.arrows }),
		Content($, { bottom: 0, left: 2, skin:assets.skins.arrows, variant:1 }),
	]
}));

class MenuTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super();
		let up = screen.first;
		let down = up.next;
		let item = screen.last;
		if (controller.going != direction)
			this.from(item, { x:screen.x - item.width }, 250, Math.quadEaseOut, 0);
		else
			this.from(item, { x:screen.x + item.width }, 250, Math.quadEaseOut, 0);
		this.from(up, { y:screen.y - up.height }, 125, Math.quadEaseOut, 0);
		this.from(down, { y:screen.y + screen.height }, 125, Math.quadEaseOut, -125);
	}
}

export default class extends View {
	static get Behavior() { return MenuBehavior }	
	constructor(data) {
		super(data);
		this.index = 0;
		this.count = data.items.length;
		this.items = data.items;
	}
	get Template() { return MenuContainer }
	get Timeline() { return MenuTimeline }
};
